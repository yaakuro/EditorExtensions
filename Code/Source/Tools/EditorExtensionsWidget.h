
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QWidget>

namespace EditorExtensions
{
    class EditorExtensionsWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit EditorExtensionsWidget(QWidget* parent = nullptr);
    };
} 
