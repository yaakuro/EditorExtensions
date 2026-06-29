/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

#include <QPointer>

class QMainWindow;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class HotKeyManagerInterface;
    class MenuManagerInterface;
    class ToolBarManagerInterface;
} // namespace AzToolsFramework

namespace EditorExtensions
{
    class ViewportScreenshotCapture;

    //! Registers the viewport screenshot action, resolution submenu, and toolbar entry, and runs captures.
    //! Mirrors the engine's EditorActionsHandler screenshot wiring, but lives entirely in this gem.
    class ViewportScreenshotActionHandler
        : private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
        , private AzToolsFramework::EditorEventsBus::Handler
    {
    public:
        void Activate();
        void Deactivate();

    private:
        // ActionManagerRegistrationNotificationBus overrides...
        void OnActionRegistrationHook() override;
        void OnMenuRegistrationHook() override;
        void OnMenuBindingHook() override;
        void OnToolBarBindingHook() override;

        // EditorEventsBus overrides...
        void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

        void CaptureViewportScreenshot(int width, int height);
        void RefreshScreenshotResolutionActions();

        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
        AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;

        // Safe against the main window being destroyed before an in-flight capture reports back.
        QPointer<QMainWindow> m_mainWindow;

        // Sole owner of the in-flight capture; its destructor tears down the pipeline if one is active.
        AZStd::unique_ptr<ViewportScreenshotCapture> m_viewportScreenshotCapture;
        bool m_screenshotAntialiasing = true;
        // Selected capture resolution. A width/height of 0 means "use the viewport resolution".
        int m_screenshotWidth = 0;
        int m_screenshotHeight = 0;
        bool m_screenshotUseCustom = false;
    };
} // namespace EditorExtensions
