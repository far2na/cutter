#include "EditVariablesDialog.h"
#include "ui_EditVariablesDialog.h"

#include <QMetaType>
#include <QComboBox>
#include <QMetaType>
#include <QPushButton>

EditVariablesDialog::EditVariablesDialog(RVA offset, QString initialVar, QWidget *parent)
    : QDialog(parent), ui(new Ui::EditVariablesDialog), functionAddress(RVA_INVALID)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditVariablesDialog::applyFields);
    connect<void (QComboBox::*)(int)>(ui->dropdownLocalVars, &QComboBox::currentIndexChanged, this,
                                      &EditVariablesDialog::updateFields);

    QString fcnName = Core()->cmdRawAt("afn.", offset).trimmed();
    functionAddress = offset;
    setWindowTitle(tr("Edit Variables in Function: %1").arg(fcnName));

    variables = Core()->getVariables(offset);
    int currentItemIndex = -1;
    int index = 0;
    for (const VariableDescription &var : variables) {
        ui->dropdownLocalVars->addItem(var.name, QVariant::fromValue(var));
        if (var.name == initialVar) {
            currentItemIndex = index;
        }
        index++;
    }
    ui->dropdownLocalVars->setCurrentIndex(currentItemIndex);
    if (currentItemIndex != -1) {
        ui->nameEdit->setFocus();
    }

    populateTypesComboBox();
    updateFields();
}

EditVariablesDialog::~EditVariablesDialog()
{
    delete ui;
}

bool EditVariablesDialog::empty() const
{
    return ui->dropdownLocalVars->count() == 0;
}

void EditVariablesDialog::applyFields()
{
    if (ui->dropdownLocalVars->currentIndex() < 0) {
        // nothing was selected or list is empty
        return;
    }
    VariableDescription desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();

    Core()->cmdRaw(QString("afvt %1 %2").arg(desc.name).arg(ui->typeComboBox->currentText()));

    // TODO Remove all those replace once rizin command parser is fixed
    QString newName = ui->nameEdit->text()
                              .replace(QLatin1Char(' '), QLatin1Char('_'))
                              .replace(QLatin1Char('\\'), QLatin1Char('_'))
                              .replace(QLatin1Char('/'), QLatin1Char('_'));
    if (newName != desc.name) {
        Core()->renameFunctionVariable(newName, desc.name, functionAddress);
    }

    // Refresh the views to reflect the changes to vars
    emit Core()->refreshCodeViews();
}

void EditVariablesDialog::updateFields()
{
    bool hasSelection = ui->dropdownLocalVars->currentIndex() >= 0;
    auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(hasSelection);
    if (!hasSelection) {
        ui->nameEdit->clear();
        return;
    }
    VariableDescription desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();
    ui->nameEdit->setText(desc.name);
    ui->typeComboBox->setCurrentText(desc.type);
}

static void addTypeDescriptionsToComboBox(QComboBox *comboBox, QList<TypeDescription> list) {
    for (const TypeDescription &thisType : list) {
        comboBox->addItem(thisType.type);
    }
    comboBox->insertSeparator(comboBox->count());
}

void EditVariablesDialog::populateTypesComboBox()
{
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllStructs());
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllPrimitiveTypes());
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllEnums());
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllTypedefs());

}
