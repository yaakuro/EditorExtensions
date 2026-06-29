/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tools/Screenshot/ViewportScreenshotActionHandler.h>

#include <Tools/Screenshot/ScreenshotCustomResolutionDialog.h>
#include <Tools/Screenshot/ViewportScreenshotCapture.h>
#include <Tools/Screenshot/ViewportScreenshotUtils.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/parallel/atomic.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>

#include <QByteArray>
#include <QDateTime>
#include <QMainWindow>

namespace
{
    // Gem-local identifiers so this feature doesn't collide with (or depend on) the engine-side
    // registration while both exist; the engine copy can be removed independently.
    constexpr AZStd::string_view ScreenshotActionIdentifier = "o3de.action.editorExtensions.viewport.captureScreenshot";
    constexpr AZStd::string_view ScreenshotViewportResolutionActionIdentifier =
        "o3de.action.editorExtensions.viewport.captureScreenshot.viewport";
    constexpr AZStd::string_view ScreenshotCustomActionIdentifier = "o3de.action.editorExtensions.viewport.captureScreenshot.custom";
    constexpr AZStd::string_view ScreenshotAntialiasingActionIdentifier =
        "o3de.action.editorExtensions.viewport.captureScreenshot.antialiasing";
    constexpr AZStd::string_view ScreenshotMenuIdentifier = "o3de.menu.editorExtensions.viewport.screenshot";

    AZStd::string ScreenshotResolutionActionIdentifier(int width, int height)
    {
        return AZStd::string::format("o3de.action.editorExtensions.viewport.captureScreenshot.resolution[%i:%i]", width, height);
    }

    // Snapshot resolution presets (width x height), grouped by aspect ratio.
    constexpr int ScreenshotResolutions[][2] = {
        // 16:9
        { 1280, 720 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 },
        // 16:10
        { 1920, 1200 }, { 2560, 1600 },
        // 21:9
        { 3440, 1440 }
    };

    // Hard cap on one capture dimension, mirroring the Editor's CustomResolutionDlg::MAX_RES.
    constexpr uint32_t MaxScreenshotDimension = 8192;
} // namespace

namespace EditorExtensions
{
    void ViewportScreenshotActionHandler::Activate()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        m_hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        m_toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();

        if (m_actionManagerInterface && m_menuManagerInterface && m_toolBarManagerInterface && m_hotKeyManagerInterface)
        {
            AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
        }

        AzToolsFramework::EditorEventsBus::Handler::BusConnect();
    }

    void ViewportScreenshotActionHandler::Deactivate()
    {
        AzToolsFramework::EditorEventsBus::Handler::BusDisconnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

        // unique_ptr: ViewportScreenshotCapture's destructor tears down any in-flight capture.
        m_viewportScreenshotCapture.reset();
    }

    void ViewportScreenshotActionHandler::NotifyMainWindowInitialized(QMainWindow* mainWindow)
    {
        m_mainWindow = mainWindow;
    }

    void ViewportScreenshotActionHandler::OnActionRegistrationHook()
    {
        // Capture Viewport Screenshot
        {
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Capture Viewport Screenshot";
            actionProperties.m_description = "Save an image of the current viewport to disk using the selected resolution.";
            actionProperties.m_category = "View";
            actionProperties.m_iconPath = ":/EditorExtensions/snapshot.svg";
            actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;
            actionProperties.m_toolBarVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

            m_actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                AZStd::string(ScreenshotActionIdentifier),
                actionProperties,
                [this]
                {
                    CaptureViewportScreenshot(m_screenshotWidth, m_screenshotHeight);
                });

            m_hotKeyManagerInterface->SetActionHotKey(AZStd::string(ScreenshotActionIdentifier), "Ctrl+Shift+S");
        }

        // Screenshot resolution selection (radio): the capture button uses the current choice.
        {
            // Viewport resolution (0x0 means "use the viewport size").
            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Viewport Resolution";
                actionProperties.m_category = "View";
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterCheckableAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    AZStd::string(ScreenshotViewportResolutionActionIdentifier),
                    actionProperties,
                    [this]
                    {
                        m_screenshotWidth = 0;
                        m_screenshotHeight = 0;
                        m_screenshotUseCustom = false;
                        RefreshScreenshotResolutionActions();
                    },
                    [this]() -> bool
                    {
                        return !m_screenshotUseCustom && m_screenshotWidth <= 0 && m_screenshotHeight <= 0;
                    });
            }

            // Fixed resolutions.
            for (const auto& resolution : ScreenshotResolutions)
            {
                const int width = resolution[0];
                const int height = resolution[1];
                const AZStd::string actionIdentifier = ScreenshotResolutionActionIdentifier(width, height);

                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = AZStd::string::format("%i x %i", width, height);
                actionProperties.m_category = "View";
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterCheckableAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this, width, height]
                    {
                        m_screenshotWidth = width;
                        m_screenshotHeight = height;
                        m_screenshotUseCustom = false;
                        RefreshScreenshotResolutionActions();
                    },
                    [this, width, height]() -> bool
                    {
                        return !m_screenshotUseCustom && m_screenshotWidth == width && m_screenshotHeight == height;
                    });
            }

            // Custom resolution (stays selected until another option is picked).
            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Custom...";
                actionProperties.m_category = "View";
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterCheckableAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    AZStd::string(ScreenshotCustomActionIdentifier),
                    actionProperties,
                    [this]
                    {
                        const auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get();
                        auto viewportContext = viewportContextRequests ? viewportContextRequests->GetDefaultViewportContext() : nullptr;
                        const AzFramework::WindowSize viewportSize =
                            viewportContext ? viewportContext->GetViewportSize() : AzFramework::WindowSize{};

                        const int defaultWidth =
                            m_screenshotWidth > 0 ? m_screenshotWidth : aznumeric_cast<int>(viewportSize.m_width);
                        const int defaultHeight =
                            m_screenshotHeight > 0 ? m_screenshotHeight : aznumeric_cast<int>(viewportSize.m_height);

                        ScreenshotCustomResolutionDialog resolutionDialog(defaultWidth, defaultHeight, m_mainWindow);
                        if (resolutionDialog.exec() == QDialog::Accepted)
                        {
                            m_screenshotWidth = resolutionDialog.GetWidth();
                            m_screenshotHeight = resolutionDialog.GetHeight();
                            m_screenshotUseCustom = true;
                            RefreshScreenshotResolutionActions();
                        }
                    },
                    [this]() -> bool
                    {
                        return m_screenshotUseCustom;
                    });
            }
        }

        // Capture Viewport Screenshot - antialiasing option
        {
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Antialiasing (SMAA)";
            actionProperties.m_category = "View";
            actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

            m_actionManagerInterface->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                AZStd::string(ScreenshotAntialiasingActionIdentifier),
                actionProperties,
                [this]
                {
                    m_screenshotAntialiasing = !m_screenshotAntialiasing;
                    m_actionManagerInterface->UpdateAction(AZStd::string(ScreenshotAntialiasingActionIdentifier));
                },
                [this]() -> bool
                {
                    return m_screenshotAntialiasing;
                });
        }
    }

    void ViewportScreenshotActionHandler::OnMenuRegistrationHook()
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Screenshot Resolution";
        m_menuManagerInterface->RegisterMenu(AZStd::string(ScreenshotMenuIdentifier), menuProperties);
    }

    void ViewportScreenshotActionHandler::OnMenuBindingHook()
    {
        m_menuManagerInterface->AddActionToMenu(AZStd::string(ScreenshotMenuIdentifier), AZStd::string(ScreenshotActionIdentifier), 100);
        m_menuManagerInterface->AddSeparatorToMenu(AZStd::string(ScreenshotMenuIdentifier), 200);

        // Resolution selection (radio); capture always uses the current choice.
        m_menuManagerInterface->AddActionToMenu(
            AZStd::string(ScreenshotMenuIdentifier), AZStd::string(ScreenshotViewportResolutionActionIdentifier), 300);

        int sortKey = 400;
        for (const auto& resolution : ScreenshotResolutions)
        {
            m_menuManagerInterface->AddActionToMenu(
                AZStd::string(ScreenshotMenuIdentifier), ScreenshotResolutionActionIdentifier(resolution[0], resolution[1]), sortKey);
            sortKey += 100;
        }

        m_menuManagerInterface->AddActionToMenu(
            AZStd::string(ScreenshotMenuIdentifier), AZStd::string(ScreenshotCustomActionIdentifier), sortKey);

        // Capture options
        m_menuManagerInterface->AddSeparatorToMenu(AZStd::string(ScreenshotMenuIdentifier), sortKey + 100);
        m_menuManagerInterface->AddActionToMenu(
            AZStd::string(ScreenshotMenuIdentifier), AZStd::string(ScreenshotAntialiasingActionIdentifier), sortKey + 200);

        // Expose the submenu in the viewport options menu, next to the engine's entry while it exists.
        m_menuManagerInterface->AddSubMenuToMenu(
            EditorIdentifiers::ViewportOptionsMenuIdentifier, AZStd::string(ScreenshotMenuIdentifier), 901);
    }

    void ViewportScreenshotActionHandler::OnToolBarBindingHook()
    {
        m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
            EditorIdentifiers::ViewportTopToolBarIdentifier,
            AZStd::string(ScreenshotActionIdentifier),
            AZStd::string(ScreenshotMenuIdentifier),
            651);
    }

    void ViewportScreenshotActionHandler::RefreshScreenshotResolutionActions()
    {
        m_actionManagerInterface->UpdateAction(AZStd::string(ScreenshotViewportResolutionActionIdentifier));
        for (const auto& resolution : ScreenshotResolutions)
        {
            m_actionManagerInterface->UpdateAction(ScreenshotResolutionActionIdentifier(resolution[0], resolution[1]));
        }
        m_actionManagerInterface->UpdateAction(AZStd::string(ScreenshotCustomActionIdentifier));
    }

    void ViewportScreenshotActionHandler::CaptureViewportScreenshot(int width, int height)
    {
        const auto viewportContextRequests = AZ::RPI::ViewportContextRequests::Get();
        const AZ::RPI::ViewportContextPtr viewportContext =
            viewportContextRequests ? viewportContextRequests->GetDefaultViewportContext() : nullptr;
        if (!viewportContext)
        {
            AZ_Warning("EditorExtensions", false, "Failed to capture viewport screenshot: no viewport context is available.");
            return;
        }

        // Keep the scene alive via ScenePtr so async teardown can't hit a dangling pointer.
        AZ::RPI::ScenePtr scene = viewportContext->GetRenderScene();
        AZ::RPI::ViewPtr sourceView = viewportContext->GetDefaultView();
        if (!scene || !sourceView)
        {
            AZ_Warning("EditorExtensions", false, "Failed to capture viewport screenshot: no scene or view is available.");
            return;
        }

        // Resolves the requested resolution, falling back to the viewport size and enforcing the dimension cap.
        const AzFramework::WindowSize viewportSize = viewportContext->GetViewportSize();
        const AZ::Outcome<EditorViewportScreenshot::CaptureResolution, AZStd::string> resolution =
            EditorViewportScreenshot::ResolveCaptureResolution(
                width, height, viewportSize.m_width, viewportSize.m_height, MaxScreenshotDimension);
        if (!resolution.IsSuccess())
        {
            AZ_Warning("EditorExtensions", false, "Failed to capture viewport screenshot: %s", resolution.GetError().c_str());
            return;
        }
        const uint32_t captureWidth = resolution.GetValue().m_width;
        const uint32_t captureHeight = resolution.GetValue().m_height;

        // Reject the request while another capture is still in flight.
        if (m_viewportScreenshotCapture && m_viewportScreenshotCapture->IsCapturing())
        {
            AZ_Warning("EditorExtensions", false, "A viewport screenshot capture is already in progress.");
            return;
        }

        // Materialized as locals so the string views passed below outlive the call.
        const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        const QByteArray timestampUtf8 = timestamp.toUtf8();
        const AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        static AZStd::atomic<uint32_t> s_screenshotSequence = 0;
        const AZStd::string screenshotPath = EditorViewportScreenshot::MakeCaptureOutputPath(
            AZStd::string_view(projectPath.c_str()),
            AZStd::string_view(timestampUtf8.constData(), timestampUtf8.size()),
            captureWidth,
            captureHeight,
            s_screenshotSequence.fetch_add(1));

        m_viewportScreenshotCapture.reset(
            new ViewportScreenshotCapture(m_mainWindow ? m_mainWindow->statusBar() : nullptr, screenshotPath, m_screenshotAntialiasing));

        if (!m_viewportScreenshotCapture->Begin(scene, sourceView, captureWidth, captureHeight))
        {
            // The unique_ptr releases and destroys the already torn-down capture.
            m_viewportScreenshotCapture.reset();
        }
    }
} // namespace EditorExtensions
