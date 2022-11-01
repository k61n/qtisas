#ifndef _COMBO_BOX_IN_TABLE_H
#define _COMBO_BOX_IN_TABLE_H

#include <qobject.h>
#include <qcombobox.h>
#include <qtablewidget.h>


class QComboBoxInTable : public QComboBox
{
    Q_OBJECT
public:
    QComboBoxInTable( int row, int column, QWidget * parent, const char * name = 0 ) ;
    
signals:
    void activated (int row, int col);
    
    public slots:
    void reEmitActivated(const QString &);
    
private:
    int row;
    int column;
};

#endif
