/***************************************************************************
 File : dan18-process-matrix.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface: matrix related tools
 
 ***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/
#include "dan18.h"

#include "deadTimeRoot.h"
#include <gsl/gsl_roots.h>
#ifdef TIFFTIFF
#include <tiffio.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>

//*******************************************
//+++  new-daDan:: find Matrix List By Label
//*******************************************
void dan18::findMatrixListByLabel(QString winLabelMask,QStringList  &listMask)
{
    listMask.clear();
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( w->metaObject()->className() == "Matrix")
    {
        if (w->windowLabel().contains(winLabelMask))
        listMask<<w->name();
    }
}

//*******************************************
//+++  new-daDan:: find Table List By Label
//*******************************************
void dan18::findTableListByLabel(QString winLabel,QStringList  &list)
{
    list.clear();
    //+++
    QList<MdiSubWindow*> windows = app()->tableList();
    foreach(MdiSubWindow *w, windows) if ( w->windowLabel().contains(winLabel)) list<<w->name();
}

//+++ [core] create Matrix fromgsl_matrix +++
void dan18::makeMatrixSymmetric( gsl_matrix * gmatrix, QString name, QString label, int MD, bool hide)
{
    return makeMatrixSymmetric( gmatrix, name, label, MD, 1, MD, 1, MD, hide);
}

//+++ [core] create Matrix fromgsl_matrix +++
void dan18::makeMatrixSymmetric( gsl_matrix * gmatrix, QString name, QString label, int MD, double xs, double xe, double ys, double ye, bool hide)
{

    int i,j;

    //+++
    QString ss;
    //+++
    bool existYN=false;
    Matrix* m;
    
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( w->metaObject()->className() == "Matrix" && w->name()==name)
    {
        m=(Matrix *)w;
        existYN=true;
        //m->setDimensions(MD,MD);
        //m->setCoordinates(xs,xe,ys,ye);
        break;
    }
    //+++make Unique Name
    

    
    if (!existYN)
    {
        //m=app()->newMatrixHidden(name,MD,MD);
        m=app()->newMatrix(name,MD,MD);
        m->setNumericFormat('E',8);
        m->setCoordinates(xs,xe,ys,ye);
    }
    
    //+++
    
    for (i=0;i<MD;i++)
        for (j=0;j<MD;j++)
        {
            m->setText(i,j,QString::number(gsl_matrix_get(gmatrix,i,j),'E',8));
        }
    //+++ new
    m->setWindowLabel(label);
    app()->setListViewLabel(m->name(), label);
    app()->updateWindowLists(m);
    
    //+++
    m->setColumnsWidth(140);
    m->resetView();
    m->notifyChanges();
    m->notifyModifiedData();
    
    if (hide) app()->hideWindow(m);
}

//+++ [core] create Matrix from gsl_matrix-Uni++++
void dan18::makeMatrixUni( gsl_matrix * gmatrix, QString name, int xDim, int yDim, double xs, double xe, double ys, double ye,bool hide, bool maximizeNewYN)
{
    makeMatrixUni(gmatrix, name, "", xDim, yDim, xs,xe,ys,ye,hide,maximizeNewYN);
}

//+++ [core] create Matrix from gsl_matrix-Uni++++
void dan18::makeMatrixUni(gsl_matrix * gmatrix, QString name, QString label, int xDim, int yDim,bool hide, bool maximizeNewYN)
{
    //makeMatrixUni(gmatrix, name, label,xDim, yDim, 0.5,xDim+0.5,0.5,yDim+0.5);
    makeMatrixUni( gmatrix, name, label,xDim, yDim, 1,xDim,1,yDim,hide,maximizeNewYN);
}

//+++ [core] create Matrix from gsl_matrix-Uni++++
void dan18::makeMatrixUni( gsl_matrix * gmatrix, QString name, QString label, int xDim, int yDim, double xs, double xe, double ys, double ye, bool hide, bool maximizeNewYN)
{
    int i,j;
    //+++
    QString ss;
    //+++
    bool existYN=false;
    Matrix* m;
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    foreach(MdiSubWindow *w, windows) if ( w->metaObject()->className() == "Matrix" && w->name()==name)
    {
        m=(Matrix *)w;
        existYN=true;
        m->setDimensions(yDim,xDim);
        m->setCoordinates(xs,xe,ys,ye);
        break;
    }
    
    //+++make Unique Name
    
    if (!existYN)
    {
        //m=app()->newMatrixHidden(name,yDim,xDim);
        m=app()->newMatrix(name,yDim,xDim);
        app()->updateRecentProjectsList();
        m->setNumericFormat('E',8);
        m->setCoordinates(xs,xe,ys,ye);
    }
    //+++
    
    for (i=0;i<yDim;i++) for (j=0;j<xDim;j++)
    {
        m->setText(i,j,QString::number(gsl_matrix_get(gmatrix,i,j),'E',8));
    }
    
    m->setWindowLabel(label);
    app()->setListViewLabel(m->name(), label);
    app()->updateWindowLists(m);

    m->setColumnsWidth(140);
    m->resetView();
    m->notifyChanges();
    m->notifyModifiedData();
    
     if (!existYN && maximizeNewYN)  app()->maximizeWindow(m);
    
    if (!maximizeNewYN && hide) app()->hideWindow(m);
}

//+++ [core] create gsl_matrix from Matrix +++
bool dan18::make_GSL_Matrix_Symmetric( QString mName, gsl_matrix * &gmatrix, int MD)
{
    //+++
    gsl_matrix_set_all(gmatrix, 0.0);               // set all elements to 0
    
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    
    //+++
    foreach(MdiSubWindow *w, windows) if ( w->metaObject()->className() == "Matrix" && w->name()==mName)
    {
        Matrix *m=(Matrix *)w;

        if (m->numRows()==MD && m->numCols()==MD )
        {
            for (int yy=0;yy<MD;yy++) for (int xx=0;xx<MD;xx++)
            {
                gsl_matrix_set(gmatrix, yy, xx,  m->text(yy,xx).toDouble() ); //Matrix transfer
            }
            
            return true;
        }
        else
        {
            toResLog("DAN :: Check dimension in Matrix "+mName+". Sensitivity/mask is set to {1}"+"\n");
            
            return false;
        }
    }
    
    //+++ Matrix does dot exist
    toResLog("DAN :: Matrix "+mName+" does not exist, m[i,j]=0\n");
    
    //+++
    return false;
}

//+++ [core] create gsl_matrix from Matrix +++
bool dan18::make_GSL_Matrix_Uni( QString mName, gsl_matrix * &gmatrix, int &xDim, int &yDim, QString &label)
{
    //+++
    QList<MdiSubWindow*> windows = app()->windowsList();
    
    //+++
    foreach(MdiSubWindow *w, windows) if ( w->metaObject()->className() == "Matrix" && w->name()==mName)
    {
        Matrix *m=(Matrix *)w;

        yDim=m->numRows();
        xDim=m->numCols();
        label=m->windowLabel();

        if (xDim<1 || yDim<1) return false;
        
        gmatrix=gsl_matrix_calloc(yDim,xDim); //  allocate and set all 0
        
        for (int xx=0;xx<xDim;xx++) for (int yy=0;yy<yDim;yy++)
        {
            gsl_matrix_set(gmatrix, yy, xx,  m->text(yy,xx).toDouble() ); //Matrix transfer
        }
        return true;
    }
    
    return false;
}



//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrix
(
 QString Number, int DD, int RegionOfInteres, int binning, int pixelPerLine, bool XY,
 int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data
 )
{
    ImportantConstants();
    return readMatrixByName( fileNameUni( wildCard, Number), DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data );
    
}


//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrix ( QString Number, gsl_matrix* &data )
{
    int DD=comboBoxMDdata->currentText().toInt();
    
    int RegionOfInteres=spinBoxRegionOfInteres->value();
    int binning=comboBoxBinning->currentText().toInt();
    
    int pixelPerLine=spinBoxReadMatrixNumberPerLine->value();
    bool XY=checkBoxTranspose->isChecked();
    int pixelsInHeader=spinBoxHeaderNumberLines->value()+spinBoxDataHeaderNumberLines->value();
    
    // 2012 ::
    bool X2mX=checkBoxMatrixX2mX->isChecked();
    bool Y2mY=checkBoxMatrixY2mY->isChecked();
    
    return readMatrixByName( fileNameUni( wildCard, Number ), DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data );
    
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrixByName ( QString fileName, gsl_matrix* &data )
{
    
    int DD=comboBoxMDdata->currentText().toInt();
    int RegionOfInteres=spinBoxRegionOfInteres->value();
    int binning=comboBoxBinning->currentText().toInt();
    int pixelPerLine=spinBoxReadMatrixNumberPerLine->value();
    bool XY=checkBoxTranspose->isChecked();
    int pixelsInHeader=spinBoxHeaderNumberLines->value()+spinBoxDataHeaderNumberLines->value();
    
    // 2012 ::
    bool X2mX=checkBoxMatrixX2mX->isChecked();
    bool Y2mY=checkBoxMatrixY2mY->isChecked();
    
    return readMatrixByName( fileName, MD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data );
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrixByName
(QString fileName, int DD, int RegionOfInteres, int binning, int pixelPerLine,
 bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data
 )
{
    MD=lineEditMD->text().toInt();
    
    if (MD==DD)
        return readMatrixByName( fileName, DD, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data , false);
    
    if (RegionOfInteres/binning !=MD) return false;
    
    gsl_matrix *dataFull=gsl_matrix_alloc(DD,DD);
    
    
    if ( !readMatrixByName( fileName, DD, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, dataFull, false ) ){
        gsl_matrix_free(dataFull);
        return false;
    }
    
    int startIndex=(DD-RegionOfInteres)/2;
    
    double sum;
    
    for (int i=0; i<MD; i++) for (int j=0; j<MD; j++)
    {
        sum=0;
        for (int ii=0; ii<binning; ii++) for (int jj=0; jj<binning; jj++)
        {
            sum+=gsl_matrix_get(dataFull,startIndex+i*binning+ii,startIndex+j*binning+jj);
        }
        gsl_matrix_set(data, i,j,sum);
    }
    
    gsl_matrix_free(dataFull);
    return true;
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrixByName
(QString fileName, int DD, int pixelPerLine,
 bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data, bool readFrame
 )
{

    if (tiffData)
    {
#ifdef TIFFTIFF
        if (fileName.contains ( ".tif", false )) return readMatrixByNameTiff( fileName, DD, XY, X2mX, Y2mY, data );
#endif
        if (fileName.contains ( ".gz", false )) return readMatrixByNameBinaryGZipped( fileName, DD, XY, X2mX, Y2mY, data );
        if (!fileName.contains ( ".tif", false )) return readMatrixByNameImage( fileName, DD, XY, X2mX, Y2mY, data );
    }

    if (pixelPerLine==1) return readMatrixByNameOne( fileName, DD, XY, pixelsInHeader, X2mX, Y2mY, data);
    
    if (DD==pixelPerLine && pixelsInHeader==0 ) 
    {
        if (readMatrixByNameGSL (fileName, data ) )
        {
            if (XY) gsl_matrix_transpose (data);
            if (X2mX) gslMatrixX2mX(data);
            if (Y2mY) gslMatrixY2mY(data);
            
            return true;
        }
        else return false;
    }
    
    
    QFile file( fileName );
    QTextStream t( &file );
    
    if (!file.open(QIODevice::ReadOnly) ) return false;
    
    
    double linesToRead=double(DD*DD)/double(pixelPerLine);
    int fullLines=int(linesToRead);
    int lastLineNumber=0;
    if (linesToRead>fullLines)
    {
        lastLineNumber=DD*DD-fullLines*pixelPerLine;
    }
    
    if (flexiHeader && flexiStop[0]!="" && !readFrame)
    {
        QString sTmp;
        int symbolsNumber;
        bool endReached=false;
        for (int i=0;i<pixelsInHeader;i++)
        {
            sTmp=t.readLine();
            
            for (int iFlex=0; iFlex<flexiStop.count(); iFlex++)
            {
                symbolsNumber=flexiStop[iFlex].length();
                if (sTmp.left(symbolsNumber)==flexiStop[iFlex] || t.atEnd() )
                {
                    endReached=true;
                    break; // skip header
                }
            }
            if (endReached) break;
        }
    }
    else for (int i=0;i<pixelsInHeader;i++) t.readLine(); // skip header
    
    QStringList lst;
    QString s;
    
    int errorsCounter=0;
    
    int currentX=0;
    int currentY=0;
    
    //+++++++++++++++++++++++++++++++++++++++++++
    for (int i=0;i<fullLines;i++)
    {
        lst.clear();
        
        s=t.readLine();
        s=s.replace(",", " "); // new :: bersans
        s=s.simplifyWhiteSpace();
        
        lst=lst.split(" ", s );
        
        if (lst.count()!=pixelPerLine)
        {
            toResLog("File :: "+fileName+" has Error at "+ QString::number(i)+" line. Numbers :: "+QString::number(lst.count())+"\n");
            errorsCounter++;
            if (errorsCounter<DD)
            {
                i=i-1; break;
            }
            else
            {
                file.close();
                return false;
            }
        }
        
        for (int j=0; j<pixelPerLine;j++)
        {
            gsl_matrix_set(data,currentY,currentX, lst[j].toDouble());
            currentX++;
            
            if (currentX>=DD)
            {
                currentX=0;
                currentY++;
            }
        }
    }
    
    if (lastLineNumber>0)
    {
        lst.clear();
        
        s=t.readLine();
        s=s.replace(",", " "); // new :: bersans
        s=s.simplifyWhiteSpace();
        
        lst=lst.split(" ", s );
        
        
        for (int j=0;j<lastLineNumber;j++)
        {
            gsl_matrix_set(data,DD-1, DD-lastLineNumber+j, lst[j].toDouble());
        }
    }
    //+++++++++++++++++++++++++++++++++++++++++++
    
    
    file.close();
    
    if (XY) gsl_matrix_transpose (data);
    if (X2mX) gslMatrixX2mX(data);
    if (Y2mY) gslMatrixY2mY(data);
    
    return true;
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
#ifdef TIFFTIFF
bool dan18::readMatrixByNameTiff
(QString fileName, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix
 )
{
    
    TIFFErrorHandler oldhandler;
    oldhandler = TIFFSetWarningHandler(NULL);
    TIFF* tif = TIFFOpen( fileName, "r");
    TIFFSetWarningHandler(oldhandler);
    
    if (!tif) return false;
    
    tdata_t buf;
    uint32 w, h;
    
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    
    buf = _TIFFmalloc(TIFFScanlineSize(tif));
    
    uint16* data;
    
    int ih0=0;
    int jw0=0;
    
    if (h<=DD) ih0+=(DD-h)/2;
    else h=DD;
    
    if (w<DD) jw0+=(DD-w)/2;
    else w=DD;
    
    for (int ih = 0; ih < h; ih++)
    {
        TIFFReadScanline(tif, buf, ih, 0);
        data=(uint16*)buf;
        
        for (int jw = 0; jw < w; jw++) if (data[2*jw]<65534) gsl_matrix_set(matrix, ih+ih0, jw+jw0, data[2*jw]);
        
    }
    
    _TIFFfree(buf);
    TIFFClose(tif);
    
    if (XY) gsl_matrix_transpose (matrix);
    if (X2mX) gslMatrixX2mX(matrix);
    if (Y2mY) gslMatrixY2mY(matrix);
    
    return true;
}
#endif

//+++++  FUNCTIONS::Read-DAT-files:: Matrix double +++++++++++++++++++++
bool dan18::readMatrixByNameImage
(QString fn, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix
 )
{
    
    bool readbleImage=fn.contains ( ".jpg", false ) ||  fn.contains ( ".bmp", false ) || fn.contains ( ".pbm", false ) ||fn.contains ( ".pgm", false );
    readbleImage=readbleImage || fn.contains ( ".png", false ) || fn.contains ( ".ppm", false ) || fn.contains ( ".xbm", false ) || fn.contains ( ".xpm", false );
    
  //  if ( !readbleImage) return false;
    if ( !readbleImage) return readMatrixFromBiniryFile(fn, DD, XY, X2mX, Y2mY, matrix);
    QPixmap photo;
    
    if ( fn.contains ( ".jpg", false ) )
        photo.load ( fn,"JPEG",QPixmap::Auto );
    else
    {
        QList<QByteArray> lst=QImageWriter::supportedImageFormats();
        for ( int i=0;i< ( int ) lst.count();i++ )
        {
            if ( fn.contains ( "." + QString(lst[i].data()), false ) )
            {
                photo.load ( fn,QString(lst[i].data()),QPixmap::Auto );
                break;
            }
        }
    }
    QImage image=photo.convertToImage();
    QSize size=photo.size();
    int w=size.width();
    int h=size.height();
    
    
    int ih0=0;
    int jw0=0;
    
    if (h<=DD) ih0+=(DD-h)/2;
    else h=DD;
    
    if (w<DD) jw0+=(DD-w)/2;
    else w=DD;
    
    for (int ih = 0; ih < h; ih++) for (int jw = 0; jw < w; jw++)
    {
        QRgb pixel = image.pixel ( jw, ih );
        gsl_matrix_set(matrix, ih+ih0, jw+jw0, qGray ( pixel ));
    }
    
    if (XY) gsl_matrix_transpose (matrix);
    if (X2mX) gslMatrixX2mX(matrix);
    if (Y2mY) gslMatrixY2mY(matrix);
    
    return true;
}
//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrixFromBiniryFile
(QString fn, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix
 )
{
    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.read(DD*DD*sizeof(int));
    int array[DD*DD];
    memcpy(&array, data.constData(), data.size());
    
    for (int i = 0; i < DD; i++)for (int j = 0; j < DD; j++) { gsl_matrix_set(matrix, i, j, array[i*DD+j]);};
    
    if (XY) gsl_matrix_transpose (matrix);
    if (X2mX) gslMatrixX2mX(matrix);
    if (Y2mY) gslMatrixY2mY(matrix);
    file.close();
    return true;
}



//+++ code from: Georg Brandl
//+++++  FUNCTIONS::Read-DAT-files:: gzipped binary ++++++++++++++++++++
bool dan18::readMatrixByNameBinaryGZipped
(QString fn, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix
 )
{
    int ROI=spinBoxRegionOfInteres->value();
    
    gzFile fd = gzopen(fn, "rb");

    int read = 0;
    int INITIAL=DD*DD;
    int rest = 4*INITIAL;
    
    char *buf = (char*)malloc(4*INITIAL);
    
    do {
        if (!rest)
        {
            buf = (char*)realloc(buf, 2*read);
            rest = read;
        }
        int neww = gzread(fd, &buf[read], rest);
        rest -= neww;
        read += neww;
    } while (!gzeof(fd));
    
    int xOffset=0;
    int yOffset=0;

    if (read/4.0==ROI*DD) xOffset=(DD-ROI)/2.0;
    else if (read/4.0<ROI*ROI) { xOffset=(DD-ROI)/2.0; yOffset=(DD-read/4.0/ROI)/2.0;};
    
    uint32_t *intbuf = (uint32_t *)buf;

    for (int yy = yOffset; yy < DD-yOffset; yy++) for (int xx = xOffset; xx < DD-xOffset; xx++)
    {
        gsl_matrix_set(matrix, yy, xx, intbuf[(yy-yOffset)*(DD-2*xOffset) + (xx-xOffset)]);
    };

    
    if (XY) gsl_matrix_transpose (matrix);
    if (X2mX) gslMatrixX2mX(matrix);
    if (Y2mY) gslMatrixY2mY(matrix);
    return true;
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrixByNameOne
(QString fileName, int DD, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data
 )
{
    QFile file( fileName );
    QTextStream t( &file );
    
    if (!file.open(QIODevice::ReadOnly) ) return false;
    
    int linesToRead=DD*DD;
    
    if (flexiHeader && flexiStop[0]!="")
    {
        QString sTmp;
        int symbolsNumber;
        bool endReached=false;
        for (int i=0;i<pixelsInHeader;i++)
        {
            sTmp=t.readLine();
            
            for (int iFlex=0; iFlex<flexiStop.count(); iFlex++)
            {
                symbolsNumber=flexiStop[iFlex].length();
                if (sTmp.left(symbolsNumber)==flexiStop[iFlex] || t.atEnd() )
                {
                    endReached=true;
                    break; // skip header
                }
            }
            if (endReached) break;
        }
    }
    else for (int i=0;i<pixelsInHeader;i++) t.readLine(); // skip header
    
    QString s;
    int currentX;
    int currentY;
    
    currentX=0;
    currentY=0;
    
    //+++++++++++++++++++++++++++++++++++++++++++
    for (int i=0;i<linesToRead;i++)
    {
        s=t.readLine().simplifyWhiteSpace();
        gsl_matrix_set(data,currentY,currentX, s.toDouble());
        currentX++;
        
        if (currentX>=DD)
        {
            currentX=0;
            currentY++;
        }
    }
    
    file.close();
    
    if (XY) gsl_matrix_transpose (data);
    if (X2mX) gslMatrixX2mX(data);
    if (Y2mY) gslMatrixY2mY(data);
    
    return true;
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readMatrixByNameGSL (QString fileName, gsl_matrix* &data )
{
    FILE * f = fopen(fileName, "r");
    gsl_set_error_handler_off ();
    gsl_matrix_fscanf (f, data);
    fclose (f);
    return true;
}

//+++++  FUNCTIONS::Read-DAT-files:: Matrix ouble +++++++++++++++++++++
bool dan18::readFromEnd (int M, gsl_matrix* &data )
{
    gsl_matrix *temp=gsl_matrix_alloc(M,M);
    
    gsl_matrix_memcpy(temp,data);
    
    for(int i=0; i<M; i++) for(int j=0; j<M; j++)
    {
        gsl_matrix_set(data,i,j,gsl_matrix_get(temp, M-i-1, M-j-1));
    }
    
    gsl_matrix_free(temp);
    
    return true;
}


//+++ Any Matrix should be read by this function!!!!
//+++
void dan18::readMatrixCor( QString Number,  gsl_matrix* &data )
{
    int DD=comboBoxMDdata->currentText().toInt();
    int RegionOfInteres=spinBoxRegionOfInteres->value();
    int binning=comboBoxBinning->currentText().toInt();
    
    int pixelPerLine=spinBoxReadMatrixNumberPerLine->value();
    bool XY=checkBoxTranspose->isChecked();
    int pixelsInHeader=spinBoxHeaderNumberLines->value()+spinBoxDataHeaderNumberLines->value();
    
    // 2012 ::
    bool X2mX=checkBoxMatrixX2mX->isChecked();
    bool Y2mY=checkBoxMatrixY2mY->isChecked();
    
    
    readMatrixCor( Number, DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data);
}


//+++ 2012 : BC time normalization
//+++
void dan18::readMatrixCorTimeNormalizationOnly( QString Number,  gsl_matrix* &data )
{
    int DD=comboBoxMDdata->currentText().toInt();
    int RegionOfInteres=spinBoxRegionOfInteres->value();
    int binning=comboBoxBinning->currentText().toInt();
    
    int pixelPerLine=spinBoxReadMatrixNumberPerLine->value();
    bool XY=checkBoxTranspose->isChecked();
    int pixelsInHeader=spinBoxHeaderNumberLines->value()+spinBoxDataHeaderNumberLines->value();
    
    // 2012 ::
    bool X2mX=checkBoxMatrixX2mX->isChecked();
    bool Y2mY=checkBoxMatrixY2mY->isChecked();
    
    readMatrixCorTimeNormalizationOnly( Number, DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data);
}


//+++ Any Matrix should be read by this function!!!!
//+++
void dan18::readMatrixCor
(
 QString Number, int DD, int RegionOfInteres, int binning,
 int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data
 )
{
    readMatrix( Number, DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data ); // read bare matrix
    
    //+++ Dead Time Correction
    double deadTimeCor=readDataDeadTime( Number );
    
    
    if (comboBoxDTtype->currentIndex()>0 && MD==128 && binning==1 )
    {
        deadtimeMatrix(Number, data);
        deadTimeCor=1;
    }
    
    //Normalization constant
    double normalization=readDataNormalization( Number );
    
    //RT- Normalization constant
    double RTnormalization=readDataNormalizationRT( Number );
    
    //+++
    int ii,jj;
    double current=0;
    //+++
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        current=gsl_matrix_get(data,jj,ii);      // jj =>y, ii=>x
        
        current*=deadTimeCor;
        
        current*=normalization;
        
        current*=RTnormalization;
        
        gsl_matrix_set(data,jj,ii,current);
    }
}

void dan18::readMatrixCorTimeNormalizationOnly
(
 QString Number, int DD, int RegionOfInteres, int binning,
 int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data
 )
{
    readMatrix( Number, DD, RegionOfInteres, binning, pixelPerLine, XY, pixelsInHeader, X2mX, Y2mY, data ); // read bare matrix
    
    //+++ Dead Time Correction
    double deadTimeCor=readDataDeadTime( Number );
    
    
    if (comboBoxDTtype->currentIndex()>0 && MD==128 && binning==1 )
    {
        deadtimeMatrix(Number, data);
        deadTimeCor=1;
    }
    
    //Normalization constant
    double normalization=spinBoxNorm->value();
    double time=readDuration( Number );
    if (time>0.0) normalization/=time; else normalization=0;
    
    
    //RT- Normalization constant
    double RTnormalization=readDataNormalizationRT( Number );
    
    //+++
    int ii,jj;
    double current=0;
    //+++
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        current=gsl_matrix_get(data,jj,ii);      // jj =>y, ii=>x
        
        current*=deadTimeCor;
        
        current*=normalization;
        
        current*=RTnormalization;
        
        gsl_matrix_set(data,jj,ii,current);
    }
}



//+++ Any Matrix should be read by this function!!!!
void dan18::parallaxCorrection(gsl_matrix* &data, double Xc, double Yc,
                               double D, double Tr)
{
    int MD=lineEditMD->text().toInt(); // +++ move to ()
    //+++
    int ii,jj;
    double current=0;
    double theta, costheta, cosalpha;
    double pixel=lineEditResoPixelSize->text().toDouble();
    double pixelAsymetry  = lineEditAsymetry->text().toDouble();
    
    double binning=comboBoxBinning->currentText().toDouble();
    
    bool trCorrectionYN=checkBoxParallaxTr->isChecked();
    bool paralaxYN=checkBoxParallax->isChecked();
    
    
    double aaa, TrRealFactor;
    

        for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
        {
            theta=atan( binning*pixel*sqrt( (Xc-ii)*(Xc-ii) + pixelAsymetry*pixelAsymetry*(Yc-jj)*(Yc-jj)) / D );
            costheta=cos(theta);
            cosalpha=cos (  asin ( pixelAsymetry * (Yc-jj) / D * costheta ) );

            if (theta>0.01)
            {
            current=gsl_matrix_get(data,jj,ii);  // jj =>y, ii=>x
            if (paralaxYN)
            {
                if (comboBoxParallax->currentIndex()==0) current = current / pow(costheta, 3.0);
                else current = current/cosalpha/costheta/costheta;
            }
            if (trCorrectionYN && Tr<1.0 && Tr>0.0 )
            {
                aaa=1.0/costheta-1.0;
                TrRealFactor=(pow(Tr,fabs(aaa)) - 1.0)/log(Tr)/fabs(aaa);
                current = current / TrRealFactor;
            }
            
            gsl_matrix_set(data,jj,ii,current);
            }
        }
}


//+++ Transmission Correction T=T(theta)
void dan18::transmissionThetaDependenceTrEC(gsl_matrix* &EC, double Xc, double Yc, double D, double Tr)
{
    int MD=lineEditMD->text().toInt(); // +++ move to ()
    
    //+++
    int ii,jj;
    double current=0.0;
    double theta;
    double pixel=lineEditResoPixelSize->text().toDouble();
    double pixelAsymetry  = lineEditAsymetry->text().toDouble();
    double binning=comboBoxBinning->currentText().toDouble(); 
    double aaa, TrReal;
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        theta=atan(binning*pixel*sqrt( (Xc-ii)*(Xc-ii) + pixelAsymetry*pixelAsymetry*(Yc-jj)*(Yc-jj))/D);
        if (theta>0.01)
        {
            current=gsl_matrix_get(EC,jj,ii);    // jj =>y, ii=>x
            aaa=1.0/cos(theta)-1.0;
            TrReal=( pow (Tr,fabs(aaa) ) - 1.0)/log(Tr)/fabs(aaa);
            current *= TrReal;
            
            gsl_matrix_set(EC,jj,ii,current);
        }
    }
}

void dan18::gslMatrixVShift(gsl_matrix *gmatrix,  int MD, int VShift)
{
    if (VShift==0 || fabs(double(VShift))>=MD-5) return;
    
    gsl_matrix *temp=gsl_matrix_alloc(MD,MD);
    gsl_matrix_memcpy(temp,gmatrix);
    
    if (VShift>0)
    {
        for (int i=VShift;i<MD;i++)
            for (int j=0;j<MD;j++)
            {
                gsl_matrix_set(gmatrix,i,j,gsl_matrix_get(temp,i-VShift,j ));
            }
    }
    else
    {
        for (int i=MD+VShift-1;i>=0;i--)
            for (int j=0;j<MD;j++){
                gsl_matrix_set(gmatrix,i,j,gsl_matrix_get(temp,i-VShift,j ));
            }
    }
    
    gsl_matrix_free(temp);
}

void dan18::gslMatrixHShift(gsl_matrix * gmatrix,  int MD, int HShift)
{
    if (HShift==0 || fabs(double(HShift))>=MD-5) return;
    
    gsl_matrix *temp=gsl_matrix_alloc(MD,MD);
    gsl_matrix_memcpy(temp,gmatrix);
    
    if (HShift>0)
    {
        for (int j=HShift;j<MD;j++)
            for (int i=0;i<MD;i++)
            {
                gsl_matrix_set(gmatrix,i,j,gsl_matrix_get(temp,i,j-HShift ));
            }
    }
    else
    {
        for (int j=MD+HShift-1;j>=0;j--)
            for (int i=0;i<MD;i++)
            {
                gsl_matrix_set(gmatrix,i,j,gsl_matrix_get(temp,i,j-HShift ));
            }
    }
    
    gsl_matrix_free(temp);
}

void dan18::gslMatrixShift(gsl_matrix * gmatrix,  int MD, double HShift, double VShift)
{
    
    if (HShift==0.0 && VShift==0.0) return;
    if (HShift==0.0 && VShift-int(VShift)==0.0) return gslMatrixVShift(gmatrix,  MD, VShift);
    if (VShift==0.0 && HShift-int(HShift)==0.0) return gslMatrixHShift(gmatrix,  MD, HShift);
    if (HShift-int(HShift)==0.0 && VShift-int(VShift)==0.0)
    {
        gslMatrixHShift(gmatrix,  MD, HShift);
        gslMatrixVShift(gmatrix,  MD, VShift);
        return;
    }
    
    gsl_matrix *temp=gsl_matrix_alloc(MD,MD);
    gsl_matrix_memcpy(temp,gmatrix);
    
    int ix1, iy1,ix2,iy2;
    double rex, rey, wt, twt, avg;
    
    
    for (int i=0;i<MD;i++) for (int j=0;j<MD;j++)
    {
        
        
        ix1 = int( double(j) - HShift );
        iy1 = int( double(i) - VShift );
        
        ix2 = ix1 + 1;
        iy2 = iy1 + 1;
        
        rex = j - HShift-ix1;
        rey = i - VShift-iy1 ;
        
        if (  ix1>=0 && ix2<MD && iy1>=0 && iy2<MD )
        {
            
            twt=0.0;
            avg=0.0;
            //+++
            wt  = (1.0-rex)*(1.0-rey);
            twt += wt;
            avg += wt *gsl_matrix_get(temp,iy1,ix1);
            
            wt  = (rex)*(1.0-rey);
            twt +=  wt;
            avg += wt*gsl_matrix_get(temp,iy1,ix2);
            
            wt  = (1.0-rex)*(rey);
            twt += wt;
            avg += wt*gsl_matrix_get(temp,iy2,ix1);
            
            wt  = (rex)*(rey);
            twt += wt;
            avg += wt * gsl_matrix_get(temp,iy2,ix2);
            
            gsl_matrix_set (gmatrix, i, j, avg/twt);
            
        }
        else gsl_matrix_set (gmatrix, i, j, 0.0);
    }
    
    gsl_matrix_free(temp);
}


//+++++SLOT::Save Sensitivity Matrx to file +++++++++++++++++++++++++++++
void dan18::saveMatrixAsTableToFile(QString fname, gsl_matrix *i, gsl_matrix *di, gsl_matrix *sigmaMa, gsl_matrix *mask, int MaDe, double xCenter, double yCenter, double wl, double dwl, double d, double xPixel, double yPixel)
{
    int format=comboBoxIxyFormat->currentIndex();
    bool SASVIEW=false;
    if (checkBoxASCIIheaderSASVIEW->isChecked()) SASVIEW=true;
    bool HEADER=false;
    if (checkBoxASCIIheaderIxy->isChecked()) HEADER=true;
    bool IGNOREMASK=false;
    if (checkBoxASCIIignoreMask->isChecked()) IGNOREMASK=true;
    bool FOCUSING=false;
    if (checkBoxResoFocus->isChecked()) FOCUSING=true;

    double sigmaDWL=0.0;
    double sigma=0.0;
    double maskValue=0;
//    xCenter-=1.0;
//    yCenter-=1.0;
    
    
    
    if ( !fname.isEmpty() )
    {
        QFile f( fname );
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            //*************************************Log Window Output
            toResLog("DAN :: saveMatrixToFile | Could not write to file::" +  fname+"\n");
            //*************************************Log Window Output
        }
        else
        {
            QTextStream stream( &f );

            if (HEADER)
            {
                if (format==4) stream<<"Q"<<"\t\t"<<"phi"<<"\t\t";
                else if (format==1) stream<<"x"<<"\t\t"<<"y"<<"\t\t";
                else stream<<"Qx"<<"\t\t"<<"Qy"<<"\t\t";
                stream<<"I[x,y]"<<"\t\t"<<"dI[x,y]";
                if (format==3) stream<<"\t\t"<<"Qz"<<"\t"<<QChar(963)<<"Q(para)"<<"\t"<<QChar(963)<<"Q(perp)"<<"\t"<<"mask";
                stream<<"\n";
            }

            if (SASVIEW) stream<<"ASCII data\n";
            
            double Q, Qx,Qy,phi;
            for(int iy=0;iy<MaDe;iy++)
            {
                for(int jx=0;jx<MaDe;jx++)
                {
                    maskValue=gsl_matrix_get(mask,iy,jx);
                    if (maskValue==0.0 && !IGNOREMASK) continue;
                    
                    
                    
                    if (format==4)
                    {
                        // polar coordinates
                        Qx= (xCenter-jx)*xPixel/d;
                        Qy=(yCenter-iy)*yPixel/d;
                        Q=sqrt(Qx*Qx+Qy*Qy);
                        if (Q==0) phi=0;
                        else if (Qx<0 && Qy<=0) phi=asin(fabs(Qx)/Q)/M_PI*180;
                        else if (Qx<=0 && Qy>0) phi=180-asin(fabs(Qx)/Q)/M_PI*180;
                        else if (Qx>0 && Qy>=0) phi=180+asin(fabs(Qx)/Q)/M_PI*180;
                        else if (Qx>=0 && Qy<0) phi=360-asin(fabs(Qx)/Q)/M_PI*180;
                        
                        Q=4*M_PI/wl*sin(0.5*atan(Q));
                        
                        stream<<QString::number(Q,'E',8)<<"\t";
                        stream<<QString::number(phi,'E',8)<<"\t";
                    }
                    else if (format==1)
                    {
                        stream<<QString::number(jx+1,'E',8)<<"\t";
                        stream<<QString::number(iy+1,'E',8)<<"\t";
                    }
                    else
                    {
                        stream<<QString::number(-4*M_PI/wl*sin(0.5*asin((xCenter-jx)*xPixel/d)),'E',8)<<"\t";
                        stream<<QString::number(-4*M_PI/wl*sin(0.5*asin((yCenter-iy)*yPixel/d)),'E',8)<<"\t";
                    }
                    
                    
                    stream<<QString::number(gsl_matrix_get(i,iy,jx),'E',8)<<"\t";
                    stream<<QString::number(gsl_matrix_get(di,iy,jx),'E',8);
                    
                    if (format==3)
                    {
                        Qx=(xCenter-jx)*xPixel/d;
                        Qy=(yCenter-iy)*yPixel/d;
                        Q=sqrt(Qx*Qx+Qy*Qy);
                        Q=4*M_PI/wl*sin(0.5*atan(Q));
                        sigmaDWL=0.4246609*Q*dwl;
                        sigma=gsl_matrix_get(sigmaMa,iy,jx);
                        stream<<"\t"<<QString::number(0.0,'f',3);
                        stream<<"\t"<<QString::number(sigma*maskValue,'E',8);
                        if (FOCUSING) stream<<"\t"<<QString::number(sigmaDWL*maskValue,'E',8);
                        else stream<<"\t"<<QString::number(sqrt(maskValue*(sigma*sigma-sigmaDWL*sigmaDWL)),'E',8);
                        stream<<"\t"<<QString::number(maskValue,'f',0);
                    }
                    
                    stream<<"\n";
                }
            }
            f.close();
        }
    }
}


//  ROI
bool dan18::extractROI(gsl_matrix *bigMatrix, gsl_matrix *smallMatrix, int xFirst, int yFirst, int xLast, int yLast  )
{
    if (xFirst<0 || xLast<0 || yFirst<0 || yLast<0) return false;
    if (xFirst>xLast || yFirst>yLast) return false;
    
    int xDimBigMartix=bigMatrix->size2;
    int yDimBigMartix=bigMatrix->size1;
    
    if ( xLast >= xDimBigMartix) return false;
    if ( yLast >= yDimBigMartix) return false;
    
    int xDimSmallMartix=smallMatrix->size2;
    int yDimSmallMartix=smallMatrix->size1;
    
    if ( xLast-xFirst > xDimSmallMartix) return false;
    if ( yLast-yFirst > yDimSmallMartix) return false;
    
    for (int i=0;i<=(xLast-xFirst);i++) for(int j=0;j<=(yLast-yFirst);j++)
    {
        gsl_matrix_set(smallMatrix,j,i,gsl_matrix_get(bigMatrix,yFirst+j,xFirst+i));
    }
    
    return true;
}

// insert matrix in matrix
bool dan18::insertMatrixInMatrix(gsl_matrix *bigMatrix, gsl_matrix *smallMatrix, int xFirst, int yFirst)
{
    int xDimBigMartix=bigMatrix->size2;
    int yDimBigMartix=bigMatrix->size1;
    
    int xDimSmallMartix=smallMatrix->size2;
    int yDimSmallMartix=smallMatrix->size1;
    
    if ( xFirst+xDimSmallMartix>xDimBigMartix) return false;
    if ( yFirst+yDimSmallMartix>yDimBigMartix) return false;
    
    for (int i=0;i<xDimSmallMartix;i++) for(int j=0;j<yDimSmallMartix;j++)
    {
        gsl_matrix_set(bigMatrix,yFirst+j,xFirst+i,gsl_matrix_get(smallMatrix,j,i));
    }
    
    return true;
}

//  generate matrix of matrixes
bool dan18::genetateMatrixInMatrix(QStringList selectedFiles, gsl_matrix *bigMatrix, int xFirst, int yFirst, int xLast, int yLast, int cols, int rrInit, int ccInit, int numberMatrixesInit)
{
    int numberMatrixes=selectedFiles.count()+numberMatrixesInit;
    int rows=int(numberMatrixes/cols);
    if (rows*cols<numberMatrixes) rows++;
    
    gsl_matrix *smallMatrix=gsl_matrix_calloc(yLast-yFirst+1, xLast-xFirst+1);
    
    int currentMatrix=numberMatrixesInit;
    QString currentName, Number;
    gsl_matrix *currentGslMatrix=gsl_matrix_alloc(MD,MD);
    
    
    //+++ mask gsl matrix
    QString maskName=comboBoxMaskFor->currentText();
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_all(mask, 1.0);
    
    if (checkBoxBigMatrixMask->isChecked()) make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sens gsl matrix
    QString sensName=comboBoxSensFor->currentText();
    gsl_matrix *sens=gsl_matrix_alloc(MD,MD);
    gsl_matrix_set_all(sens, 1.0);
    if (checkBoxBigMatrixSens->isChecked())     make_GSL_Matrix_Symmetric( sensName, sens, MD);
    
    for(int rr=rrInit;rr<rows;rr++) for (int cc=ccInit;cc<cols;cc++)
    {
        if (currentMatrix>=numberMatrixes) break;
        currentName=selectedFiles[currentMatrix-numberMatrixesInit];
        
        Number=selectedFiles[currentMatrix-numberMatrixesInit];
        Number=findFileNumberInFileName(wildCard,Number.remove(Dir));
        
        gsl_matrix_set_zero(currentGslMatrix);
        
        
        
        if (checkBoxBigMatrixASCII->isChecked())
        {
            readMatrixByNameGSL (selectedFiles[currentMatrix-numberMatrixesInit], currentGslMatrix );
        }
        else
        {
            if (checkBoxBigMatrixNorm->isChecked())
                readMatrixCor(Number,  currentGslMatrix );
            else
                readMatrix(Number,  currentGslMatrix );
        }
        
        if (checkBoxBigMatrixMask->isChecked())
            gsl_matrix_mul_elements(currentGslMatrix,mask);
        
        if (checkBoxBigMatrixSens->isChecked())
            gsl_matrix_mul_elements(currentGslMatrix,sens);
        
        if (!extractROI(currentGslMatrix, smallMatrix, xFirst, yFirst, xLast, yLast  ))
        {
            currentMatrix++;
            continue;
        };
        
        if (!insertMatrixInMatrix(bigMatrix, smallMatrix, (currentMatrix-int(currentMatrix/cols)*cols)*(xLast-xFirst+1), int(currentMatrix/cols)*(yLast-yFirst+1)))
        {
            currentMatrix++;
            continue;
        };
        currentMatrix++;
    }
    
    gsl_matrix_free(smallMatrix);
    gsl_matrix_free(currentGslMatrix);
    gsl_matrix_free(mask);
    gsl_matrix_free(sens);
    
    return true;
}

void dan18::gslMatrixX2mX(gsl_matrix *&m)
{
    int sizeY=m->size1;
    for (int c=0;c<int(sizeY/2);c++)  gsl_matrix_swap_columns (m, c, sizeY-c-1);
}

void dan18::gslMatrixY2mY(gsl_matrix *&m)
{
    int sizeX=m->size2;
    for (int r=0;r<int(sizeX/2);r++)  gsl_matrix_swap_rows (m, r, sizeX-r-1);
}

void dan18::deadtimeMatrix( QString Number, gsl_matrix* &data)
{
    if (data->size1!=MD || data->size2!=MD) return;
    
    // Detector Structure :: 13-17-17-17-17-17-17-13
    // 1x photo :: tau matrix 8 x 8 :: dead-time per  photom...
    // 9x photo::
    
    gsl_matrix* tau=gsl_matrix_alloc(8,8);
    gsl_matrix* tau9=gsl_matrix_alloc(8,8);
    gsl_matrix* dataCorr=gsl_matrix_alloc(MD,MD);
    
    
    int startI, startJ;
    int finishI, finishJ;
    int numberI, numberJ;
    
    double sum;
    
    //+++ sum near one photomultiplayer
    for (int i=0; i<8;i++) for (int j=0; j<8;j++)
    {
        if (i==0 || i==7) numberI=13; else numberI=17;
        if (j==0 || j==7) numberJ=13; else numberJ=17;
        
        if (i>0 ) startI=13+17*(i-1); else startI=0;
        if (j>0 ) startJ=13+17*(j-1); else startJ=0;
        
        sum=0;
        
        for (int ii=0; ii<numberI;ii++) for (int jj=0; jj<numberJ;jj++)
        {
            sum+=gsl_matrix_get(data,startI+ii,startJ+jj);
        }
        gsl_matrix_set(tau, i,j,sum);
        
    }
    //+++ Dead Time per whole detector
    double deadTime=lineEditDeadTime->text().toDouble();
    
    //+++ in case we assume 9x
    if (comboBoxDTtype->currentIndex()==2)
    {
        for (int i=0; i<8;i++) for (int j=0; j<8;j++)
        {
            
            if (i==0) startI=0; else startI=i-1;
            if (j==0) startJ=0; else startJ=j-1;
            
            if (i==7) finishI=7; else finishI=i+1;
            if (j==7) finishJ=7; else finishJ=j+1;
            
            
            sum=0;
            
            for (int ii=startI; ii<=finishI;ii++) for (int jj=startJ; jj<=finishJ;jj++)
            {
                sum+=gsl_matrix_get(tau,ii,jj);
            }
            gsl_matrix_set(tau9, i,j,sum);
            
        }
        
        gsl_matrix_memcpy(tau,tau9);
        deadTime*=4;
    }
    else if (comboBoxDTtype->currentIndex()==1) deadTime*=64;
    
    double time=readDuration( Number );
    
    for (int i=0; i<8;i++) for (int j=0; j<8;j++)
    {
        double dtCor=deadTimeFaktor(gsl_matrix_get(tau,i,j) /time , deadTime);
        gsl_matrix_set(tau,i,j,dtCor);
    }
    
    for (int i=0; i<8;i++) for (int j=0; j<8;j++)
    {
        if (i==0 || i==7) numberI=13; else numberI=17;
        if (j==0 || j==7) numberJ=13; else numberJ=17;
        
        if (i>0 ) startI=13+17*(i-1); else startI=0;
        if (j>0 ) startJ=13+17*(j-1); else startJ=0;
        
        for (int ii=0; ii<numberI;ii++) for (int jj=0; jj<numberJ;jj++)
        {
            gsl_matrix_set(dataCorr,startI+ii,startJ+jj, gsl_matrix_get(data,startI+ii,startJ+jj)* gsl_matrix_get(tau, i,j));
        }
        
    }
    gsl_matrix_memcpy(data,dataCorr);
    
    gsl_matrix_free(tau);
    gsl_matrix_free(tau9);
    gsl_matrix_free(dataCorr);
}

//+++++SLOT::Save Sensitivity Matrx to file +++++++++++++++++++++++++++++
void dan18::saveMatrixToFile(QString fname, gsl_matrix *m, int MaDe)
{
    //+++
    int ii, jj;
    
    if ( !fname.isEmpty() )
    {
        QFile f( fname );
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            //*************************************Log Window Output
            toResLog("DAN :: saveMatrixToFile | Could not write to file::" +  fname+"\n");
            //*************************************Log Window Output
        }
        else
        {
            QTextStream stream( &f );
            for(ii=0;ii<MaDe;ii++)
            {
                for(jj=0;jj<MaDe;jj++)
                {
                    stream<<QString::number(gsl_matrix_get(m,ii,jj),'E',8);
                    stream<<" ";
                }
                stream<<"\n";
            }
            f.close();
        }
    }
}

//+++++SLOT::Save Sensitivity Matrx to file +++++++++++++++++++++++++++++
void dan18::saveMatrixToFileInteger(QString fname, gsl_matrix *m, int MaDe)
{
    //+++
    int ii, jj;
    
    if ( !fname.isEmpty() )
    {
        QFile f( fname );
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            //*************************************Log Window Output
            toResLog("DAN :: saveMatrixToFile | Could not write to file::" +  fname);
            //*************************************Log Window Output
        }
        else
        {
            QTextStream stream( &f );
            for(ii=0;ii<MaDe;ii++)
            {
                for(jj=0;jj<MaDe;jj++)
                {
                    stream<<"\t"<<QString::number(int(gsl_matrix_get(m,ii,jj)));
                }
                stream<<"\n";
            }
            f.close();
        }
    }
}

//+++++SLOT::Save Sensitivity Matrx to file +++++++++++++++++++++++++++++
void dan18::saveMatrixToFile(QString fname, gsl_matrix *m, int MaDeY, int MaDeX)
{
    //+++
    int ii, jj;
    
    if ( !fname.isEmpty() )
    {
        QFile f( fname );
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            //*************************************Log Window Output
            toResLog("DAN :: saveMatrixToFile | Could not write to file::" +  fname+"\n");
            //*************************************Log Window Output
        }
        else
        {
            QTextStream stream( &f );
            for(ii=0;ii<MaDeY;ii++)
            {
                for(jj=0;jj<MaDeX;jj++)
                {
                    stream<<QString::number(gsl_matrix_get(m,ii,jj),'E',8);
                    stream<<" ";
                }
                stream<<"\n";
            }
            f.close();
        }
    }
}

//+++++FUNCTIONS::Read-DAT-files:: Matrix Double
void dan18::readErrorMatrix( QString Number, gsl_matrix* &error )
{
    readMatrix( Number, error);
    //+++
    int ii,jj;
    double current=0;
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        current=gsl_matrix_get(error,jj,ii); // jj =>y, ii=>x
        
        if (current>0) current=1.0/fabs(current);
        else current=0.0;
        gsl_matrix_set(error,jj,ii,current);
    }
}


//+++++FUNCTIONS::Read-DAT-files:: Matrix Double
void dan18::readErrorMatrixRel( QString Number, gsl_matrix* &error )
{
    readMatrix( Number, error);
    
    //+++
    int ii,jj;
    double current=0.0;
    
    for(ii=0;ii<MD;ii++) for(jj=0;jj<MD;jj++)
    {
        current=gsl_matrix_get(error,jj,ii); // jj =>y, ii=>x
        
        if (current>0) current=1.0/sqrt(fabs(current));
        else current=0.0;
        gsl_matrix_set(error,jj,ii,current);
    }
}

//+++ Integral intensity  over mask pixels:: Double ::
double dan18::integralVSmaskUni(gsl_matrix *sample, gsl_matrix *mask, int MaDe)
{
    double integral=0;
    for (int i=0;i<MaDe;i++) for (int j=0;j<MaDe;j++)
    {
        integral+=gsl_matrix_get(sample,i,j)*gsl_matrix_get(mask,i,j);
    }
    return integral;
}


//+++ integralVSmaskSimmetrical
double dan18::integralVSmaskSimmetrical( QString Number )
{
    updateMaskList();
    QString maskName=comboBoxMaskFor->currentText();
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sample gsl matrix
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    readMatrix( Number, sample);
    
    double res=integralVSmaskUni(sample, mask, MD);
    
    gsl_matrix_free(sample);
    gsl_matrix_free(mask);
    
    return res;
}


//+++
double dan18::Q2_VS_maskSimmetrical( QString Number, bool showLogYN )
{
    //+++ preparation to non-quadratic matrixes
    int MDx=MD;
    int MDy=MD;
    //---
    
    updateMaskList();
    QString maskName=comboBoxMaskFor->currentText();
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MDy,MDx);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sample gsl matrix
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    readMatrix( Number, sample);
    
    
    double D=readDataD( Number );
    double lambda=readLambda( Number );
    double pixel=lineEditResoPixelSize->text().toDouble();
    double binning=comboBoxBinning->currentText().toDouble();
    
    double Q=2*M_PI/lambda*pixel*binning/D;
    
    //double xC=(MD-1)/2;
    //double yC=(MD-1)/2;
    
    double xC=(spinBoxRBxBS->value()+spinBoxLTxBS->value())/2.0-1.0; //2022
    double yC=(spinBoxLTyBS->value()+spinBoxRByBS->value())/2.0-1.0; //2022
    
    double integral=0;
    for (int i=0;i<MD;i++) for (int j=0;j<MD;j++)
    {
        integral+=Q*Q*((i-yC)*(i-yC)+(j-xC)*(j-xC))*gsl_matrix_get(sample,i,j)*gsl_matrix_get(mask,i,j);
    }
    
    gsl_matrix_free(mask);
    gsl_matrix_free(sample);
    
    QString string2log="\nFast Info Extractor :: Q2-vs-Mask :: \n";
    string2log+="["+Number+"]:  Q2 integral = "+QString::number(integral)+"\n";
    string2log+="SDD="+QString::number(D)+"cm  lambda="+QString::number(lambda)+"A  pixel="+QString::number(pixel*binning)+"cm\n";
    string2log+="xC="+QString::number(xC+1)+"  yC="+QString::number(yC+1)+"  mask="+maskName+"\n\n";
    
    //toResLog(string2log);
    if (showLogYN) std::cout<<string2log.latin1();
    
    return integral;
}

//+++
double dan18::integralVSmaskUniDeadTimeCorrected(QString Number)
{
    updateMaskList();
    QString maskName=comboBoxMaskFor->currentText();
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    //+++ sample gsl matrix
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    readMatrix( Number, sample );
    
    if (comboBoxDTtype->currentIndex()>0 && MD==128)
    {
        deadtimeMatrix( Number, sample);
    }
    else
    {
        double deadTimeCor=readDataDeadTime( Number );
        gsl_matrix_scale(sample,deadTimeCor);
    }
    
    double res=integralVSmaskUni(sample, mask, MD);
    
    gsl_matrix_free(mask);
    gsl_matrix_free(sample);
    
    return res;
}


//+++
double dan18::integralVSmaskUniDeadTimeCorrected(QString Number, QString maskName, double VShift, double HShift)
{
    updateMaskList();
    
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    make_GSL_Matrix_Symmetric( maskName, mask, MD);
    
    double res=integralVSmaskUniDeadTimeCorrected(Number, mask, VShift, HShift);
    
    gsl_matrix_free(mask);
    
    return res;
}

//+++ 2021
double dan18::integralVSmaskUniDeadTimeCorrected(QString Number, gsl_matrix *mask, double VShift, double HShift)
{
    
    //+++ sample gsl matrix
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    readMatrix( Number, sample );
    gslMatrixShift(sample, MD, HShift, VShift );
    
    if (comboBoxDTtype->currentIndex()>0 && MD==128)
    {
        deadtimeMatrix( Number, sample);
    }
    else
    {
        double deadTimeCor=readDataDeadTime( Number );
        gsl_matrix_scale(sample,deadTimeCor);
    }
    
    
    double res=integralVSmaskUni(sample, mask, MD);
    
    gsl_matrix_free(sample);
    
    return res;
}


QString dan18::integralVSmaskUniByName(QString fileNumber)
{
    ImportantConstants();
    
    QString maskName=comboBoxMaskFor->currentText();
    //+++ mask gsl matrix
    gsl_matrix *mask=gsl_matrix_alloc(MD,MD);
    make_GSL_Matrix_Symmetric(maskName, mask, MD);
    
    //+++ sample gsl matrix
    gsl_matrix *sample=gsl_matrix_alloc(MD,MD);
    readMatrixByName( fileNumber, sample);
    
    double res=integralVSmaskUni(sample, mask, MD);
    
    gsl_matrix_free(mask);
    gsl_matrix_free(sample);
    
    return QString::number( res );
}

//+++ uni by default
//+++ exp- dead-time
double dan18::deadTimeFaktor(double I, double tau)
{
    double tauInt=tau;
    double res;
    
    if (tau<=0.0) return 1.0;
    
    if (radioButtonDeadTimeDet->isChecked())
    {
        if (I>(0.99/tauInt)) res=100.0;
        else res=1.0/(1.0-tauInt*I);
    }
    else
    {
        
        if (I>(1/tauInt/exp(1.0))) res=exp(1.0);
        else if (I==0.0) res=0.0;
        else
        {
            int status, status1;
            int iter = 0, max_iter = 100;
            
            const gsl_root_fdfsolver_type *T;
            gsl_root_fdfsolver *s;
            
            double x0, x = I;
            
            gsl_function_fdf FDF;
            
            struct deadTimeRoot_params params = {I, tauInt};
            FDF.f = &deadTimeRoot_f;
            FDF.df = &deadTimeRoot_deriv;
            FDF.fdf = &deadTimeRoot_fdf;
            FDF.params = &params;
            
            T = gsl_root_fdfsolver_newton;
            s = gsl_root_fdfsolver_alloc(T);
            
            gsl_root_fdfsolver_set (s, &FDF, x);
            
            
            do
            {
                iter++;
                status1 = gsl_root_fdfsolver_iterate (s);
                x0 = x;
                x = gsl_root_fdfsolver_root (s);
                status = gsl_root_test_delta (x, x0, 0.0, 1e-5);
            }
            while (status1 == GSL_CONTINUE && status == GSL_CONTINUE && iter
                   < max_iter);
            
            res=x/I;
            
            gsl_root_fdfsolver_free(s);
        }
    }
    
    
    return res;
}



//+++ [2017] Matrix Convolusion

int dan18::matrixConvolusion( gsl_matrix *sample, gsl_matrix *mask, int MD)
{
    int convolutionType=comboBoxMatrixConvolusion->currentIndex();
    if (convolutionType==0) return 0;
    
    gsl_matrix *sampleInit=gsl_matrix_alloc(MD,MD);
    gsl_matrix_memcpy(sampleInit, sample);
    
    
    
    int numberPixels;
    double sum;
    for (int i=0;i<MD;i++) for (int j=0;j<MD;j++)
    {
        numberPixels=0;
        sum=0.0;
        
        if (gsl_matrix_get(mask,i,j)==0) continue;
        
        sum = gsl_matrix_get(sampleInit,i,j)*gsl_matrix_get(mask,i,j); numberPixels += gsl_matrix_get(mask,i,j);
        
        if(i>0) {sum += gsl_matrix_get(sampleInit,i-1,j)*gsl_matrix_get(mask,i-1,j); numberPixels+=gsl_matrix_get(mask,i-1,j);}
        if(i<(MD-1)) {sum += gsl_matrix_get(sampleInit,i+1,j)*gsl_matrix_get(mask,i+1,j); numberPixels+=gsl_matrix_get(mask,i+1,j);}
        if(j>0) {sum += gsl_matrix_get(sampleInit,i,j-1)*gsl_matrix_get(mask,i,j-1); numberPixels+=gsl_matrix_get(mask,i,j-1);}
        if(j<(MD-1)) {sum += gsl_matrix_get(sampleInit,i,j+1)*gsl_matrix_get(mask,i,j+1); numberPixels+=gsl_matrix_get(mask,i,j+1);}

        if (convolutionType > 1 )
        {
            if(i>0 && j>0) {sum += gsl_matrix_get(sampleInit,i-1,j-1)*gsl_matrix_get(mask,i-1,j-1); numberPixels+=gsl_matrix_get(mask,i-1,j-1);}
            if(i<(MD-1) && j>0) {sum += gsl_matrix_get(sampleInit,i+1,j-1)*gsl_matrix_get(mask,i+1,j-1); numberPixels+=gsl_matrix_get(mask,i+1,j-1);}
            if(i>0 && j<(MD-1)) {sum += gsl_matrix_get(sampleInit,i-1,j+1)*gsl_matrix_get(mask,i-1,j+1); numberPixels+=gsl_matrix_get(mask,i-1,j+1);}
            if(i<(MD-1) && j<(MD-1)) {sum += gsl_matrix_get(sampleInit,i+1,j+1)*gsl_matrix_get(mask,i+1,j+1); numberPixels+=gsl_matrix_get(mask,i+1,j+1);}
        }
        
        if (convolutionType > 2 )
        {
            //+++
            if (j-2>=0) for(int ii=i-2; ii<i+2 ; ii++)
            {
                if(ii<0 || ii>= MD )continue;sum+=gsl_matrix_get(sampleInit,ii,j-2)*gsl_matrix_get(mask,ii,j-2);numberPixels+=gsl_matrix_get(mask,ii,j-2);
            }
            //+++
            if ( (j+2) < MD ) for(int ii=i-2; ii<i+2 ; ii++)
            {
                if(ii<0 || ii>= MD )continue;sum+=gsl_matrix_get(sampleInit,ii,j+2)*gsl_matrix_get(mask,ii,j+2);numberPixels+=gsl_matrix_get(mask,ii,j+2);
            }
            //+++
            if (i-2>=0) for(int jj=j-1; jj<j+1 ; jj++)
            {
                if( jj<0 || jj>=MD )continue;sum+=gsl_matrix_get(sampleInit,i-2,jj)*gsl_matrix_get(mask,i-2,jj);numberPixels+=gsl_matrix_get(mask,i-2,jj);
            }
            //+++
            if ( (i+2) < MD ) for(int jj=j-1; jj<j+1 ; jj++)
            {
                if(jj<0 || jj>=MD )continue;sum+=gsl_matrix_get(sampleInit,i+2,jj)*gsl_matrix_get(mask,i+2,jj);numberPixels+=gsl_matrix_get(mask,i+2,jj);
            }
        }
        
        if (numberPixels>0) gsl_matrix_set(sample,i,j, sum/double(numberPixels));
        else gsl_matrix_set(sample,i,j, 0.0);
    }

    gsl_matrix_free(sampleInit);
    return convolutionType;
}
