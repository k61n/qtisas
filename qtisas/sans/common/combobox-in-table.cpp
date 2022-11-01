#include "combobox-in-table.h"

QComboBoxInTable::QComboBoxInTable (int row, int column, QWidget * parent, const char * name )
: QComboBox( parent, name ), row(row), column(column)
{
    this->disconnect();
    connect( this, SIGNAL(currentIndexChanged (const QString &)), this, SLOT(reEmitActivated(const QString &) ));
}
 
void QComboBoxInTable::reEmitActivated(const QString & string)
{
    emit activated(row, column);
}
