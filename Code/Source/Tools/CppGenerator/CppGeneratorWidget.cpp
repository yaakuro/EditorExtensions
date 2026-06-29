#include "CppGeneratorWidget.h"
#include "Tools/CppGenerator/ui_CppGeneratorWidget.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QProcess>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QRadioButton>

#include <AzCore/Utils/Utils.h>

namespace EditorExtensions
{
    CppGeneratorWidget::CppGeneratorWidget(QWidget* parent) : QWidget(parent), ui(new Ui::CppGeneratorWidget)
    {
        ui->setupUi(this);

        QObject::connect(ui->pushButtonCreate, &QPushButton::clicked, this, &CppGeneratorWidget::CreateClass);
        QObject::connect(ui->toolButtonSelectDestination, &QPushButton::clicked, this, &CppGeneratorWidget::SelectDestination);
        QObject::connect(ui->radioButtonEngine, &QRadioButton::toggled, this, &CppGeneratorWidget::OnRelativeToEngineToggled);
        QObject::connect(ui->radioButtonProject, &QRadioButton::toggled, this, &CppGeneratorWidget::OnRelativeToProjectToggled);

        ui->radioButtonProject->setChecked(true);
    }

    CppGeneratorWidget::~CppGeneratorWidget()
    {
        QObject::disconnect(ui->pushButtonCreate, &QPushButton::clicked, this, &CppGeneratorWidget::CreateClass);
        QObject::disconnect(ui->toolButtonSelectDestination, &QPushButton::clicked, this, &CppGeneratorWidget::SelectDestination);
        QObject::disconnect(ui->radioButtonEngine, &QRadioButton::toggled, this, &CppGeneratorWidget::OnRelativeToEngineToggled);
        QObject::disconnect(ui->radioButtonProject, &QRadioButton::toggled, this, &CppGeneratorWidget::OnRelativeToProjectToggled);

        delete ui;
    }

    void CppGeneratorWidget::OnRelativeToEngineToggled()
    {
        m_fileRelativeDestinationPath = AZ::Utils::GetEnginePath();
        ui->lineEditDestination->setText("");
        ui->lineEditNamespace->setText("");
    }

    void CppGeneratorWidget::OnRelativeToProjectToggled()
    {
        m_fileRelativeDestinationPath = AZ::Utils::GetProjectPath();
        m_fileFinalDestinationPath = m_fileRelativeDestinationPath;

        // Set final destination path.
        AZ::IO::Path relativePath = "Gem";
        m_fileFinalDestinationPath /= relativePath;
        ui->lineEditDestination->setText(relativePath.c_str());
        ui->lineEditDestination->setProperty("destination", m_fileFinalDestinationPath.c_str());

        // Set namespace
        ui->lineEditNamespace->setText(AZ::Utils::GetProjectName().c_str());
    }

    QStringList CppGeneratorWidget::BuildCreateFromTemplateArguments() const
    {
        return {
            "create-from-template",
            "--force",
            "-dp", ui->lineEditDestination->property("destination").toString(),
            "-dn", ui->lineEditComponentName->text(),
            "-tn", "DefaultComponent",
            "-kr", "-r",
            "${GemName}", ui->lineEditNamespace->text()
        };
    }

    void CppGeneratorWidget::CreateClass()
    {
        if (ui->lineEditComponentName->text().isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter a component name."));
            return;
        }

        AZ::IO::Path scriptPath(AZ::Utils::GetEnginePath());
        scriptPath /= "scripts";
        scriptPath /= "o3de.sh";
        if (!QFile::exists(scriptPath.c_str())) {
            QMessageBox::critical(this, tr("Error"), tr("O3DE script not found at: %1").arg(scriptPath.c_str()));
            return;
        }

        QProcess process;
        process.start(scriptPath.c_str(), BuildCreateFromTemplateArguments(), QIODevice::ReadWrite);

        if (!process.waitForStarted(-1)) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to start the generator process."));
            return;
        }

        if (!process.waitForFinished(-1)) {
            QMessageBox::warning(this, tr("Error"), tr("Generator process timed out or failed."));
            return;
        }

        const QString output = QString::fromUtf8(process.readAllStandardOutput());
        ui->labelLog->setText(output.isEmpty() ? tr("No output generated.") : output);
    }

    void CppGeneratorWidget::SelectDestination()
    {
        const QString directory =
            QFileDialog::getExistingDirectory(this,
                                                tr("Select Directory"),
                                                m_fileRelativeDestinationPath.c_str(),
                                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (directory.isEmpty()) {
            return;
        }

        const QDir projectDir(m_fileRelativeDestinationPath.c_str());
        QDir selectedDir(directory);
        const QString relativePath = projectDir.relativeFilePath(selectedDir.absolutePath());
        ui->lineEditDestination->setText(relativePath);

        m_fileFinalDestinationPath = m_fileRelativeDestinationPath;

        const AZ::IO::Path relativePathAz = relativePath.toUtf8().data();
        m_fileFinalDestinationPath /= relativePathAz;
        ui->lineEditDestination->setProperty("destination", m_fileFinalDestinationPath.c_str());
    }
}