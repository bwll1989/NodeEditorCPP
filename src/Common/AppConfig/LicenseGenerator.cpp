#include "LicenseManager.h"

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {

bool isMachineCode(const QString& code)
{
    QString normalized = code.trimmed().toUpper();
    normalized.remove(QLatin1Char('-'));
    normalized.remove(QLatin1Char(' '));
    if (normalized.size() != 16) {
        return false;
    }
    for (const QChar ch : normalized) {
        if (!ch.isDigit() && (ch < QLatin1Char('A') || ch > QLatin1Char('F'))) {
            return false;
        }
    }
    return true;
}

class LicenseGeneratorWindow : public QWidget
{
public:
    explicit LicenseGeneratorWindow(const QString& initialMachineCode, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle(QStringLiteral("授权码生成"));
        setWindowIcon(QIcon(QStringLiteral(":/license/license.png")));
        setMinimumWidth(480);

        auto* root = new QVBoxLayout(this);
        auto* title = new QLabel(QStringLiteral("授权码生成"), this);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        title->setFont(titleFont);
        root->addWidget(title);

        auto* form = new QFormLayout();
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        m_machineEdit = new QLineEdit(this);
        m_machineEdit->setPlaceholderText(QStringLiteral("XXXX-XXXX-XXXX-XXXX"));
        m_machineEdit->setText(initialMachineCode);
        auto* machineRow = new QHBoxLayout();
        machineRow->addWidget(m_machineEdit, 1);
        auto* localButton = new QPushButton(QStringLiteral("本机"), this);
        machineRow->addWidget(localButton);
        form->addRow(QStringLiteral("机器码:"), machineRow);

        m_licenseEdit = new QLineEdit(this);
        m_licenseEdit->setReadOnly(true);
        auto* licenseRow = new QHBoxLayout();
        licenseRow->addWidget(m_licenseEdit, 1);
        auto* copyButton = new QPushButton(QStringLiteral("复制"), this);
        licenseRow->addWidget(copyButton);
        form->addRow(QStringLiteral("授权码:"), licenseRow);

        root->addLayout(form);

        m_status = new QLabel(this);
        root->addWidget(m_status);

        auto* generateButton = new QPushButton(QStringLiteral("生成"), this);
        root->addWidget(generateButton);

        connect(localButton, &QPushButton::clicked, this, [this]() {
            m_machineEdit->setText(LicenseManager::machineCode());
            generateLicense();
        });
        connect(generateButton, &QPushButton::clicked, this, [this]() { generateLicense(); });
        connect(m_machineEdit, &QLineEdit::returnPressed, this, [this]() { generateLicense(); });
        connect(copyButton, &QPushButton::clicked, this, [this]() {
            const QString license = m_licenseEdit->text();
            if (license.isEmpty()) {
                m_status->setText(QStringLiteral("还没有可复制的授权码"));
                return;
            }
            if (auto* clipboard = QGuiApplication::clipboard()) {
                clipboard->setText(license);
            }
            m_status->setText(QStringLiteral("授权码已复制"));
        });

        generateLicense();
    }

private:
    void generateLicense()
    {
        const QString machine = m_machineEdit->text().trimmed();
        if (!isMachineCode(machine)) {
            m_licenseEdit->clear();
            m_status->setText(QStringLiteral("机器码格式应为 XXXX-XXXX-XXXX-XXXX"));
            return;
        }
        m_licenseEdit->setText(LicenseManager::licenseFor(machine));
        m_status->setText(QStringLiteral("已生成"));
    }

    QLineEdit* m_machineEdit = nullptr;
    QLineEdit* m_licenseEdit = nullptr;
    QLabel* m_status = nullptr;
};

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/license/license.png")));

    const QStringList args = QCoreApplication::arguments();
    const QString initialMachine = args.size() >= 2 ? args.at(1).trimmed() : LicenseManager::machineCode();

    LicenseGeneratorWindow window(initialMachine);
    window.show();
    return app.exec();
}
