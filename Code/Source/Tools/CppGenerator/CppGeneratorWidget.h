#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QWidget>
#endif


QT_BEGIN_NAMESPACE
namespace Ui { class CppGeneratorWidget; }
QT_END_NAMESPACE

namespace EditorExtensions
{
    class CppGeneratorWidget : public QWidget
    {
        Q_OBJECT

        public:
        explicit CppGeneratorWidget(QWidget* parent = nullptr);

        ~CppGeneratorWidget() override;

    public slots:

        void CreateClass();
        void SelectDestination();
        void OnRelativeToEngineToggled();
        void OnRelativeToProjectToggled();

        QStringList BuildCreateFromTemplateArguments() const;

    private:

        Ui::CppGeneratorWidget* ui{};
        AZ::IO::Path m_fileRelativeDestinationPath{};
        AZ::IO::Path m_fileFinalDestinationPath{};
    };
}