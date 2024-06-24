
#ifndef COMBOBOXINTABLE_H
#define COMBOBOXINTABLE_H

#include <QComboBox>
#include <QObject>
#include <QTabWidget>

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
