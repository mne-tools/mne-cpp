//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>
#include <disp/plot.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "math.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"
#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTableWidgetItem>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>

#include "QtGui"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace UTILSLIB;
using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

MatrixXd _datas;
MatrixXd _times;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================
enum TruncationCriterion
{
    Iterations,
    SignalEnergy,
    Both
};

qreal _sollEnergy = 0;
qreal _signalEnergy = 0;
qreal _signalMaximum = 0;
qreal _signalNegativeScale = 0;
qreal _maxPos = 0;
qreal _maxNeg = 0;

QList<QColor> _colors;

MatrixXd _signalMatrix(0, 0);
MatrixXd _original_signal_Matrix(0,0);
VectorXd _globalAtomList;
VectorXd _residuumVector;
VectorXd _atomSumVector;
QList<GaborAtom> myAtomList;

QList<QStringList> globalResultAtomList;
qint32 processValue = 0;

// constructor
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->progressBarCalc->setHidden(true);

    callGraphWindow = new GraphWindow();    
    callGraphWindow->setMinimumHeight(220);
    callGraphWindow->setMinimumWidth(500);
    callGraphWindow->setMaximumHeight(220);

    callAtomSumWindow = new AtomSumWindow();
    callAtomSumWindow->setMinimumHeight(220);
    callAtomSumWindow->setMinimumWidth(500);
    callAtomSumWindow->setMaximumHeight(220);

    callResidumWindow = new ResiduumWindow();
    callResidumWindow->setMinimumHeight(220);
    callResidumWindow->setMinimumWidth(500);
    callResidumWindow->setMaximumHeight(220);

    ui->sb_Iterations->setMaximum(9999);        // max iterations
    ui->sb_Iterations->setMinimum(1);           // min iterations

    ui->l_Graph->addWidget(callGraphWindow);
    ui->l_atoms->addWidget(callAtomSumWindow);
    ui->l_res->addWidget(callResidumWindow);

    ui->splitter->setStretchFactor(1,4);
    ui->tb_ResEnergy->setText(tr("0,1"));

    ui->tbv_Results->setColumnCount(5);
    ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
    ui->tbv_Results->setColumnWidth(0,50);
    ui->tbv_Results->setColumnWidth(1,40);
    ui->tbv_Results->setColumnWidth(2,40);
    ui->tbv_Results->setColumnWidth(3,40);
    ui->tbv_Results->setColumnWidth(4,40);

    // build config file at init
    bool hasEntry1 = false;
    bool hasEntry2 = false;
    bool hasEntry3 = false;
    QString contents;

    QDir dir("Matching-Pursuit-Toolbox");
    if(!dir.exists())
        dir.mkdir(dir.absolutePath());
    QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
    if(!configFile.exists())
    {
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        configFile.close();
    }

    if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        while(!configFile.atEnd())
        {
            contents = configFile.readLine(0).constData();
            if(contents.startsWith("ShowDeleteMessageBox=") == 0)
                hasEntry1 = true;
            if(contents.startsWith("ShowProcessDurationMessageBox=") == 0)
                hasEntry2 = true;
            if(contents.startsWith("ShowDeleteFormelMessageBox=") == 0)
                hasEntry3 = true;
        }
    }
    configFile.close();

    if(!hasEntry1)
    {
        if (configFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &configFile );
            stream << QString("ShowDeleteMessageBox=true;") << "\n";
        }
        configFile.close();
    }

    if(!hasEntry2)
    {
        if (configFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &configFile );
            stream << QString("ShowProcessDurationMessageBox=true;") << "\n";
        }
        configFile.close();
    }

    if(!hasEntry3)
    {
        if (configFile.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &configFile );
            stream << QString("ShowDeleteFormelMessageBox=true;") << "\n";
        }
        configFile.close();
    }



    QStringList filterList;
    filterList.append("*.dict");
    QFileInfoList fileList =  dir.entryInfoList(filterList);

    for(int i = 0; i < fileList.length(); i++)
        ui->cb_Dicts->addItem(QIcon(":/images/icons/DictIcon.png"), fileList.at(i).baseName());


    MatrixXd m(5,3);
    m << 1, 2, 3,
         4, 5, 6,
         7, 8, 9,
        10, 11, 12,
        13, 14, 15;

    VectorXd v(3);
    v << 7, 5, 3;

    add_row_at(m, v, 2);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::open_file()
{
    QFileDialog* fileDia;
    QString fileName = fileDia->getOpenFileName(this, "Please select signalfile.",QDir::currentPath(),"(*.fif *.txt)");
    if(fileName.isNull()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Fehler"),
        tr("error: disable to open signalfile."));
        return;
    }
    file.close();

    this->model = new QStandardItemModel;
    connect(this->model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(slot_changed(const QModelIndex&, const QModelIndex&)));

    _colors.clear();
    _colors.append(QColor(0, 0, 0));
    if(fileName.endsWith(".fif", Qt::CaseInsensitive))        
    {    ReadFiffFile(fileName);
        // TODO: find good signal part
        _signalMatrix.resize(1024,5);
        _original_signal_Matrix.resize(1024,5);
        for(qint32 channels = 0; channels < 5; channels++)
        {
            _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));
            this->item = new QStandardItem;

            this->item->setText(QString("Channel %1").arg(channels));
            this->item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            this->item->setData(Qt::Checked, Qt::CheckStateRole);

            this->model->insertRow(channels, this->item);
            this->items.push_back(this->item);

            for(qint32 i = 0; i < 1024; i++)
                _signalMatrix(i, channels) = _datas(channels, i) * 100000000000;
        }
        _original_signal_Matrix = _signalMatrix;
        ui->cb_channels->setModel(this->model);
    }
    else
        ReadMatlabFile(fileName);

    update();   
}

void MainWindow::slot_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    QStandardItem* item = this->items[topLeft.row()];
    if(item->checkState() == Qt::Unchecked)
    {
       _signalMatrix = remove_column(_signalMatrix, topLeft.row());
    }
    else if(item->checkState() == Qt::Checked)
    {
       VectorXd vec = _original_signal_Matrix.col(topLeft.row());
       _signalMatrix =  add_column_at(_signalMatrix,  vec,  topLeft.row());
    }
    update();
}

MatrixXd MainWindow::add_row_at(MatrixXd& matrix, VectorXd& rowData, qint32 rowNumber)
{
   std::cout << "\n-------------before---------------\n";
   std::cout << matrix;

   matrix.conservativeResize(matrix.rows() + 1, Eigen::NoChange);

   qint32 i = rowNumber;
   while(i < matrix.rows())
   {
        RowVectorXd vec1 = VectorXd::Zero(matrix.rows());
        vec1 = matrix.row(i);
        matrix.row(i) = rowData;
        rowData = vec1;
        i++;
   }
   std::cout << "\n------------after---------------\n";
   std::cout << matrix;

   return matrix;

}

MatrixXd MainWindow::add_column_at(MatrixXd& matrix, VectorXd& rowData, qint32 colNumber)
{
   //std::cout << "\n-------------before---------------\n";
   //std::cout << matrix;

   matrix.conservativeResize(Eigen::NoChange, matrix.cols() + 1);

   qint32 i = colNumber;
   while(i < matrix.cols())
   {
        RowVectorXd vec1 = VectorXd::Zero(matrix.cols());
        vec1 = matrix.col(i);
        matrix.col(i) = rowData;
        rowData = vec1;
        i++;
   }
   //std::cout << "\n------------after---------------\n";
   //std::cout << matrix;

   return matrix;

}

MatrixXd MainWindow::remove_row(MatrixXd& matrix, qint32 rowToRemove)
{
    qint32 numRows = matrix.rows()-1;
    qint32 numCols = matrix.cols();

    if( rowToRemove < numRows )
        matrix.block(rowToRemove,0,numRows-rowToRemove,numCols) = matrix.block(rowToRemove+1,0,numRows-rowToRemove,numCols);

    matrix.conservativeResize(numRows,numCols);

    return matrix;
}

MatrixXd MainWindow::remove_column(MatrixXd& matrix, qint32 colToRemove)
{
    qint32 numRows = matrix.rows();
    qint32 numCols = matrix.cols()-1;

    if( colToRemove < numCols )
        matrix.block(0,colToRemove,numRows,numCols-colToRemove) = matrix.block(0,colToRemove+1,numRows,numCols-colToRemove);

    matrix.conservativeResize(numRows,numCols);

    return matrix;
}

qint32 MainWindow::ReadFiffFile(QString fileName)
{
    QFile t_fileRaw(fileName);

    float from = 42.956f;
    float to = 50.670f;

    bool in_samples = false;

    bool keep_comp = true;

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileRaw);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
       printf("No projector specified for these data\n");
    else
    {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
           raw.info.projs[k].active = true;

       printf("%d projection items activated\n",raw.info.projs.size());
       //
       //   Create the projector
       //
       fiff_int_t nproj = raw.info.make_projector(raw.proj);

       if (nproj == 0)
           printf("The projection vectors do not apply to these channels\n");
       else
           printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
    }

    //
    //   Set up the CTF compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = -1;

    if (current_comp > 0)
       printf("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
       qDebug() << "This part needs to be debugged";
       if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
       {
          raw.info.set_current_comp(dest_comp);
          printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
       }
       else
       {
          printf("Could not make the compensator\n");
          return -1;
       }
    }
    //
    //   Read a data segment
    //   times output argument is optional
    //
    bool readSuccessful = false;


    if (in_samples)
        readSuccessful = raw.read_raw_segment(_datas, _times, (qint32)from, (qint32)to, picks);
    else
        readSuccessful = raw.read_raw_segment_times(_datas, _times, from, to, picks);

    if (!readSuccessful)
    {
       printf("Could not read raw segment.\n");
       return -1;
    }

    printf("Read %d samples.\n",(qint32)_datas.cols());


    std::cout << _datas.block(0,0,10,10) << std::endl;

    return 0;
}

void MainWindow::ReadMatlabFile(QString fileName)
{
    QFile file(fileName);
    QString contents;
    QList<QString> strList;
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        strList.append(file.readLine(0).constData());
    }
    int rowNumber = 0;
    _signalMatrix.resize(strList.length(), 1);
    file.close();
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        contents = file.readLine(0).constData();

        bool isFloat;
        qreal value = contents.toFloat(&isFloat);
        if(!isFloat)
        {
            QString errorSignal = QString("The signal could not completly read. Line %1 from file %2 coud not be readed.").arg(rowNumber).arg(fileName);
            QMessageBox::warning(this, tr("error"),
            errorSignal);
            return;
        }
        _signalMatrix(rowNumber, 0) = value;
        rowNumber++;
    }


    file.close();
    _signalEnergy = 0;
    for(qint32 i = 0; i < _signalMatrix.rows(); i++)
        _signalEnergy += (_signalMatrix(i, 0) * _signalMatrix(i, 0));
}

//*************************************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------

void GraphWindow::paintEvent(QPaintEvent* event)
{
    PaintSignal(_signalMatrix, _residuumVector, _colors, this->size());
}

void GraphWindow::PaintSignal(MatrixXd signalMatrix, VectorXd residuumSamples, QList<QColor> colors, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(signalMatrix.rows() > 0)
    {
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = 0;                   // smalest signalvalue
        qreal maxPos = 0;                   // highest signalvalue
        qreal absMin = 0;                   // minimum of abs(maxNeg and maxPos)
        qint32 drawFactor = 0;              // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QList<QPolygonF> polygons;          // points for drawing the signal
        MatrixXd internSignalMatrix = signalMatrix; // intern representation of y-axis values of the signal (for painting only)

        // paint window white
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // find min and max of signal
        i = 0;
        while(i < signalMatrix.rows())
        {
            if(signalMatrix(i, 0) > maxPos)
                maxPos = signalMatrix(i, 0);

            if(signalMatrix(i, 0) < maxNeg )
                maxNeg = signalMatrix(i, 0);
            i++;
        }

        if(maxPos > fabs(maxNeg)) absMin = maxNeg;        // find absolute minimum of (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                   // absMin must not be zero
        {
            while(true)                                   // shift factor for decimal places?
            {
                if(fabs(absMin) < 1)                      // if absMin > 1 , no shift of decimal places nescesary
                {
                    absMin = absMin * 10;
                    drawFactor++;                         // shiftfactor counter
                }
                if(fabs(absMin) >= 1) break;
            }
        }        

        // shift of decimal places with drawFactor for all signalpoints and save to intern list
        while(drawFactor > 0)
        {

            for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)
                for(qint32 sample = 0; sample < signalMatrix.rows(); sample++)
                    internSignalMatrix(sample, channel) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;
            maxPos = maxPos * 10;
            maxNeg = maxNeg * 10;
            drawFactor--;
        }

        _maxPos = maxPos;       // to globe MAx and Minvalue;
        _maxNeg = maxNeg;

        qreal maxmax;
        // absolute signalheight
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;

        _signalMaximum = maxmax;

        // scale axis title
        qreal scaleXText = (qreal)signalMatrix.rows() / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)maxmax / (qreal)10;
        qint32 negScale =  floor((maxNeg * 10 / maxmax)+0.5);
        _signalNegativeScale = negScale;
        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 k = 0;
        qint32 negScale2 = negScale;
        qint32 maxStrLenght = 0;
        while(k < 16)
        {
            QString string2;

            qreal scaledYText = negScale2 * scaleYText / (qreal)startDrawFactor;                                     // Skalenwert Y-Achse
            string2  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(string2.length()>maxStrLenght) maxStrLenght = string2.length();

            k++;
            negScale2++;
        }
        maxStrLenght = 6 + maxStrLenght * 6;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalMatrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = negScale * scaleYText / (qreal)startDrawFactor;                                    // Skalenwert Y-Achse
            string  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(negScale == 0)                                                                                       // x-Achse erreicht (y-Wert = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)       // over all Channels
                {
                    QPolygonF poly;
                    qint32 h = 0;
                    while(h < signalMatrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }

                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    QString str;

                    painter.drawText(j * scaleXAchse + maxStrLenght - 7, -(((i - 1) * scaleYAchse)-(windowSize.height())) + xAxisTextPos, str.append(QString("%1").arg((j * scaleXText))));      // Skalenwert
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // Anstriche
                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // Skalenwert Y-Achse zeichen
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // Anstriche Y-Achse
            i++;
            negScale++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis



        for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)             // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();    
}

//*************************************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------

void AtomSumWindow::paintEvent(QPaintEvent* event)
{
   PaintAtomSum(_atomSumVector, this->size(), _signalMaximum, _signalNegativeScale);
}

void AtomSumWindow::PaintAtomSum(VectorXd signalSamples, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(signalSamples.rows() > 0)
    {
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = 0;                   // smalest signalvalue
        qreal maxPos = 0;                   // highest signalvalue
        qreal absMin = 0;                   // minimum of abs(maxNeg and maxPos)
        qint32 drawFactor = 0;              // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QPolygonF polygons;                 // points for drwing the signal
        QList<qreal> internListValue;       // intern representation of y-axis values of the signal (for painting only)

        internListValue.clear();
        while(i < signalSamples.rows())
        {
                internListValue.append(signalSamples[i]);
                i++;
        }

        // paint window white
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));
        /*
        // find min and max of signal
        i = 0;
        while(i < signalSamples.rows())
        {
            if(signalSamples[i] > maxPos)
                maxPos = signalSamples[i];

            if(signalSamples[i] < maxNeg )
                maxNeg = signalSamples[i];
            i++;
        }*/


        if(maxPos > fabs(maxNeg)) absMin = maxNeg;        // find absolute minimum of (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                   // absMin must not be zero
        {
            while(true)                                   // shift factor for decimal places?
            {
                if(fabs(absMin) < 1)                      // if absMin > 1 , no shift of decimal places nescesary
                {
                    absMin = absMin * 10;
                    drawFactor++;                         // shiftfactor counter
                }
                if(fabs(absMin) >= 1) break;
            }
        }

        // shift of decimal places with drawFactor for all signalpoints and save to intern list
        while(drawFactor > 0)
        {
            i = 0;
            while(i < signalSamples.rows())
            {
                qreal replaceValue = internListValue.at(i) * 10;
                internListValue.replace(i, replaceValue);
                i++;
            }
            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;
            maxPos = maxPos * 10;
            maxNeg = maxNeg * 10;
            drawFactor--;
        }
        maxPos = _maxPos;
        maxNeg = _maxNeg;
        // scale axis title
        qreal scaleXText = (qreal)signalSamples.rows() / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 k = 0;
        qint32 negScale2 = maxNeg;
        qint32 maxStrLenght = 0;
        while(k < 16)
        {
            QString string2;

            qreal scaledYText = negScale2 * scaleYText / (qreal)startDrawFactor;                                     // Skalenwert Y-Achse
            string2  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(string2.length()>maxStrLenght) maxStrLenght = string2.length();

            k++;
            negScale2++;
        }
        maxStrLenght = 6 + maxStrLenght * 6;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalSamples.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = signalNegativeMaximum * scaleYText / (qreal)startDrawFactor;                                    // Skalenwert Y-Achse
            string  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(signalNegativeMaximum == 0)                                                                                       // x-Achse erreicht (y-Wert = 0)
            {
                // append scaled signalpoints
                qint32 h = 0;
                while(h < signalSamples.rows())
                {
                    polygons.append(QPointF((h * scaleX) + maxStrLenght,  -((internListValue.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    h++;
                }

                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    QString str;

                    painter.drawText(j * scaleXAchse + maxStrLenght - 7, -(((i - 1) * scaleYAchse)-(windowSize.height())) + xAxisTextPos, str.append(QString("%1").arg((j * scaleXText))));      // Skalenwert
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // Anstriche
                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // Skalenwert Y-Achse zeichen
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // Anstriche Y-Achse
            i++;
            signalNegativeMaximum++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis

        painter.drawPolyline(polygons);                  // paint signal
    }
    painter.end();
    /*
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0, this->size().width(), this->size().height(),QBrush(Qt::white));


    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    //VectorXd signalSamples = signalVector;

    if(signalSamples.rows() > 0)
    {
        qint32 borderMarginHeigth = 2;      // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 2;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = 0;                   // smalest signalvalue
        qreal maxPos = 0;                   // highest signalvalue
        qreal absMin = 0;                   // minimum of abs(maxNeg and maxPos)
        qint32 drawFactor = 0;              // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QPolygonF polygons;                 // points for drwing the signal
        QList<qreal> internListValue;       // intern representation of y-axis values of the signal (for painting only)

        internListValue.clear();
        while(i < signalSamples.rows())
        {
                internListValue.append(signalSamples[i]);
                i++;
        }

        // find min and max of signal
        i = 0;
        while(i < signalSamples.rows())
        {
            if(signalSamples[i] > maxPos)
                maxPos = signalSamples[i];

            if(signalSamples[i] < maxNeg )
                maxNeg = signalSamples[i];
            i++;
        }

        if(maxPos > fabs(maxNeg)) absMin = maxNeg;      // find absolute minimum of (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                 // absMin must not be zero
        {
            while(true)                                 // shift factor for decimal places?
            {
                if(fabs(absMin) < 1)                    // if absMin > 1 , no shift of decimal places nescesary
                {
                    absMin = absMin * 10;
                    drawFactor++;                       // shiftfactor counter
                }
                if(fabs(absMin) >= 1) break;
            }
        }

        // shift of decimal places with drawFactor for all signalpoints and save to intern list
        while(drawFactor > 0)
        {
            i = 0;
            while(i < signalSamples.rows())
            {
                qreal replaceValue = internListValue.at(i) * 10;
                internListValue.replace(i, replaceValue);
                i++;
            }
            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;
            maxPos = maxPos * 10;
            maxNeg = maxNeg * 10;
            drawFactor--;
        }

        qreal maxmax;
        // absolute signalheight
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - borderMarginWidth)) / (qreal)signalSamples.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        // append scaled signalpoints
        qint32 h = 0;

        while(h < signalSamples.rows())
        {
           // polygons.append(QPointF((h * scaleX) + maxStrLenght,  -((internListValue.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
            polygons.append(QPointF(h * scaleX, -((internListValue.at(h) * scaleY - (windowSize.height()) + borderMarginHeigth / 2))));
            h++;
        }

        painter.drawPolyline(polygons);                  // paint signal
    }
    painter.restore();
    //painter->end();

    std::cout << "PaintEnde \n";
    */
}

//*************************************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------

void ResiduumWindow::paintEvent(QPaintEvent* event)
{
   PaintResiduum(_residuumVector, this->size(), _signalMaximum, _signalNegativeScale);
}

void ResiduumWindow::PaintResiduum(VectorXd signalSamples, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(signalSamples.rows() > 0)
    {
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = _maxNeg;             // smalest signalvalue vom AusgangsSignal
        qreal maxPos = _maxPos;             // highest signalvalue vom AusgangsSignal
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QPolygonF polygons;                 // points for drwing the signal
        QList<qreal> internListValue;       // intern representation of y-axis values of the signal (for painting only)

        internListValue.clear();
        while(i < signalSamples.rows())
        {
                internListValue.append(signalSamples[i]);
                i++;
        }
        // paint window white
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // scale axis title
        qreal scaleXText = (qreal)signalSamples.rows() / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 k = 0;
        qint32 negScale2 = maxNeg;
        qint32 maxStrLenght = 0;
        while(k < 16)
        {
            QString string2;

            qreal scaledYText = negScale2 * scaleYText;                                     // Skalenwert Y-Achse
            string2  = QString::number(scaledYText, 'f', decimalPlace + 1);                 // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(string2.length()>maxStrLenght) maxStrLenght = string2.length();

            k++;
            negScale2++;
        }
        maxStrLenght = 6 + maxStrLenght * 6;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalSamples.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = signalNegativeMaximum * scaleYText / (qreal)startDrawFactor;                                    // Skalenwert Y-Achse
            string  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(signalNegativeMaximum == 0)                                                                                       // x-Achse erreicht (y-Wert = 0)
            {
                // append scaled signalpoints
                qint32 h = 0;
                while(h < signalSamples.rows())
                {
                    polygons.append(QPointF((h * scaleX) + maxStrLenght,  -((internListValue.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    h++;
                }

                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    QString str;

                    painter.drawText(j * scaleXAchse + maxStrLenght - 7, -(((i - 1) * scaleYAchse)-(windowSize.height())) + xAxisTextPos, str.append(QString("%1").arg((j * scaleXText))));      // Skalenwert
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // Anstriche
                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // Skalenwert Y-Achse zeichen
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // Anstriche Y-Achse
            i++;
            signalNegativeMaximum++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis

        painter.drawPolyline(polygons);                  // paint signal
    }
    painter.end();
}

// starts MP-algorithm
void MainWindow::on_btt_Calc_clicked()
{
    TruncationCriterion criterion;
    processValue = 0;
    ui->progressBarCalc->setValue(0);
    ui->progressBarCalc->setHidden(false);

    if(ui->chb_Iterations->isChecked() && !ui->chb_ResEnergy->isChecked())
        criterion = TruncationCriterion::Iterations;
    if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        criterion = TruncationCriterion::Both;
    if(ui->chb_ResEnergy->isChecked() && !ui->chb_Iterations->isChecked())
        criterion = TruncationCriterion::SignalEnergy;

    if(_signalMatrix.rows() == 0)
    {
        QString title = "Hinweis";
        QString text = "Es wurde noch kein Signal eingelesen!";
        QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
        msgBox.exec();

        return;
    }

    if(ui->chb_Iterations->checkState()  == Qt::Unchecked && ui->chb_ResEnergy->checkState() == Qt::Unchecked)
    {
        QString title = "Fehler";
        QString text = "Es wurde kein Abbruchkriterium gewaehlt.";
        QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
        msgBox.exec();
        return;
    }

    QString res_energy_str = ui->tb_ResEnergy->text();
    res_energy_str.replace(",", ".");

    if(((res_energy_str.toFloat() <= 1 && ui->tb_ResEnergy->isEnabled()) && (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled())) || (res_energy_str.toFloat() <= 1 && ui->tb_ResEnergy->isEnabled() && !ui->sb_Iterations->isEnabled()) || (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled() && !ui->tb_ResEnergy->isEnabled()) )
    {
        QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
        bool showMsgBox = false;
        QString contents;
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            while(!configFile.atEnd())
            {
                contents = configFile.readLine(0).constData();
                if(QString::compare("ShowProcessDurationMessageBox=true;\n", contents) == 0)
                    showMsgBox = true;
            }
        }
        configFile.close();

        if(showMsgBox)
        {
            processdurationmessagebox* msgBox = new processdurationmessagebox(this);
            msgBox->setModal(true);
            msgBox->exec();
            msgBox->close();
        }
    }

    if(ui->chb_ResEnergy->isChecked())
    {
        bool ok;
        //QString res_energy_str = ui->tb_ResEnergy->text();
        //res_energy_str.replace(",", ".");
        qreal percent_value = res_energy_str.toFloat(&ok);

        if(!ok)
        {
            QString title = "Fehler";
            QString text = "Es wurde keine korrekte Zahl als Abbruchkriterium eingegeben.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            ui->tb_ResEnergy->setFocus();
            ui->tb_ResEnergy->selectAll();
            return;
        }
        if(percent_value >= 100)
        {
            QString title = "Fehler";
            QString text = "Bitte geben eine Zahl kleiner als 100% ein.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            ui->tb_ResEnergy->setFocus();
            ui->tb_ResEnergy->selectAll();
            return;
        }
        _sollEnergy =  _signalEnergy / 100 * percent_value;
    } 

    if(ui->rb_OwnDictionary->isChecked())
    {

        QFile ownDict(QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
        //residuumVector =  mpCalc(ownDict, signalVector, 0);
        update();
    }
    else if(ui->rb_adativMp->isChecked())
    {
        CalcAdaptivMP(_signalMatrix, criterion);
    }    
}

//*************************************************************************************************************

void MainWindow::iteration_counter(qint32 current_iteration, qreal current_energy)
{
    qint32 local_copy_iteration = current_iteration;
    ui->progressBarCalc->setValue(local_copy_iteration);

}

//*************************************************************************************************************

void MainWindow::CalcAdaptivMP(MatrixXd signal, TruncationCriterion criterion)
{
    //qint32 it = 1000;
    //if(criterion == TruncationCriterion::Iterations || criterion == TruncationCriterion::Both)
    //   it = iterations;

    //qreal epsilon = 0.0000000000001;
    AdaptiveMp *adaptive_Mp = new AdaptiveMp();
    qint32 t_iSize = 256;
    //MatrixXd signal (t_iSize, 1);
    MatrixXd residuum = signal;


    //Testsignal
    GaborAtom *testSignal1 = new GaborAtom();//t_iSize, 40, 180, 40, 0.8);
    VectorXd t1 = testSignal1->create_real(t_iSize, 40, 180, 40, 0.8);//Samples, Scale, translat, modulat(symmetric to size/2, phase
    VectorXd t2 = testSignal1->create_real(t_iSize, 80, 52, 50, PI);
    VectorXd t5 = testSignal1->create_real(t_iSize, 20, 57, 95.505, PI);
    VectorXd t6 = testSignal1->create_real(t_iSize, 10, 40, 120, 0.5);
    VectorXd t7 = testSignal1->create_real(t_iSize, 150, 0, 30, PI/2);
    VectorXd t8 = testSignal1->create_real(t_iSize, 256, 128, 12, 0);
    VectorXd t9 = testSignal1->create_real(t_iSize, 80, 70, 10, 2.7);
    VectorXd t3 = testSignal1->create_real(t_iSize, 180, 40, 40, -0.3);
    VectorXd t4 = testSignal1->create_real(t_iSize, 210, 200, 46, 1);
    VectorXd t0 = testSignal1->create_real(t_iSize, 56, 58, 60, 0.6);
    VectorXd tSig(t_iSize);

    /*
    for(qint32 i = 0; i < t_iSize; i++)
    {
        signal(i, 0) =  10 * t1[i] +  10 * t2[i] + 15 * t5[i] + 2 * cos(qreal(i) / 5.0);// + 10 * t8[i] + 10 * t7[i]+ 8 * t6[i] + 5 * t4[i] + 20 *t3[i]+ 11 * t0[i] + 20 * t9[i];
        //signal(i, 0) = 100* t8[i];//2 * cos(qreal(i) / 5.0);
            if(i == 149)
                signal(i, 0) += 25;
               //signal(i, 0) += 7 * (sin(qreal(i*i))/ 15.0);
    }
    */
    //find  maximum of signal
    qreal maximum = 0;

    for(qint32 i = 1; i < t_iSize; i++)
        if(abs(maximum) < abs(signal(i,0)))
            maximum = signal(i,0);
    std::cout << "hoechste Amplitude im Signal:    " << maximum << "\n";

    //plot testsignal
    for(qint32 i = 0; i < t_iSize; i++)
        tSig[i] = signal(i, 0);

    Plot *sPlot = new Plot(tSig);
    sPlot->setTitle("Signalplot");
    //sPlot->show();
    //tessignal ende

    //set and update progressbar
    ui->progressBarCalc->setMinimum(0);
    //ui->progressBarCalc->setMaximum(100);
    //ui->progressBarCalc->setValue(0.01);
    //ui->progressBarCalc->setHidden(false);

    //connect(adaptive_Mp, SIGNAL(iteration_params(qint32, qreal)), this, SLOT(iteration_counter(qint32, qreal)));

    switch(criterion)
    {
        case Iterations:
        {
            connect(adaptive_Mp, SIGNAL(iteration_params(qint32, qreal)), this, SLOT(iteration_counter(qint32, qreal)));
            ui->progressBarCalc->setMaximum(ui->sb_Iterations->value());
            myAtomList = adaptive_Mp->matching_pursuit(signal, ui->sb_Iterations->value(), qreal(MININT32));
        }
        break;

        case SignalEnergy:
        {
            connect(adaptive_Mp, SIGNAL(iteration_params(qint32, qreal)), this, SLOT(iteration_counter(qint32, qreal)));
            QString res_energy_str = ui->tb_ResEnergy->text();
            res_energy_str.replace(",", ".");
            ui->progressBarCalc->setMaximum(100 - res_energy_str.toFloat());
            myAtomList = adaptive_Mp->matching_pursuit(signal, MAXINT32, (ui->tb_ResEnergy->text().toFloat()));
        }
        break;

        case Both:
        {
            ui->progressBarCalc->setMaximum(1000);
            connect(adaptive_Mp, SIGNAL(iteration_params(qint32, qreal)), this, SLOT(iteration_counter(qint32, qreal)));
            myAtomList = adaptive_Mp->matching_pursuit(signal, ui->sb_Iterations->value(), (ui->tb_ResEnergy->text().toDouble()));
        }
        break;
    }
    //connect(adaptive_Mp, SIGNAL(iteration_params(qint32, qreal)), this, SLOT(iteration_counter(qint32, qreal)));

    //run MP Algorithm

    //ui->progressBarCalc->setValue(var);

    // results in tableView
    //************************************************************************************************************************************

    ui->tbv_Results->setRowCount(myAtomList.length());
    VectorXd signalEnergy = VectorXd::Zero(signal.cols());
    VectorXd residuumEnergy = VectorXd::Zero(signal.cols());
    //calculate signalenergy
    for(qint32 channel = 0; channel < signal.cols(); channel++)
    {
        for(qint32 sample = 0; sample < signal.rows(); sample++)
            signalEnergy[channel] += (signal(sample, channel) * signal(sample, channel));
    }

    residuumEnergy[0] = signalEnergy[0];

    for(qint32 i = 0; i < myAtomList.length(); i++)
    {
        qreal procentAtomEnergie = 0;
        if(signalEnergy[0] != 0) // TODO: Multichannel
        {
            procentAtomEnergie = 100 * myAtomList[i].energy / signalEnergy[0];
            residuumEnergy[0] -= myAtomList[i].energy;
        }

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(procentAtomEnergie, 'f', 2));
        QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(myAtomList[i].scale, 'g', 3));
        QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(myAtomList[i].translation, 'g', 3));
        QTableWidgetItem* atomModulationItem = new QTableWidgetItem(QString::number(myAtomList[i].modulation, 'g', 3));
        QTableWidgetItem* atomPhaseItem = new QTableWidgetItem(QString::number(myAtomList[i].phase, 'g', 3));


        atomEnergieItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        atomScaleItem->setFlags(Qt::ItemIsEnabled);
        atomTranslationItem->setFlags(Qt::ItemIsEnabled);
        atomModulationItem->setFlags(Qt::ItemIsEnabled);
        atomPhaseItem->setFlags(Qt::ItemIsEnabled);

        atomEnergieItem->setCheckState(Qt::Checked);

        atomEnergieItem->setTextAlignment(0x0082);
        atomScaleItem->setTextAlignment(0x0082);
        atomTranslationItem->setTextAlignment(0x0082);
        atomModulationItem->setTextAlignment(0x0082);
        atomPhaseItem->setTextAlignment(0x0082);
        ui->tbv_Results->setItem(i, 0, atomEnergieItem);
        ui->tbv_Results->setItem(i, 1, atomScaleItem);
        ui->tbv_Results->setItem(i, 2, atomTranslationItem);
        ui->tbv_Results->setItem(i, 3, atomModulationItem);
        ui->tbv_Results->setItem(i, 4, atomPhaseItem);
    }
    if(signalEnergy[0] != 0) // TODO: Multichannel
        residuumEnergy[0] = 100 * residuumEnergy[0] / signalEnergy[0];
    ui->lb_RestEnergieResiduumValue->setText(QString::number(residuumEnergy[0], 'f', 2) + "%");
    /*
    ui->lb_IterationsProgressValue->setText(QString("%1").arg(iterationsCount));
    for(qint32 i = 0; i < residuum.rows(); i++)
        residuumEnergie += residuum[i] * residuum[i];
    if(residuumEnergie == 0)    ui->lb_RestEnergieResiduumValue->setText("0%");
    else    ui->lb_RestEnergieResiduumValue->setText(QString("%1%").arg(residuumEnergie / signalEnergie * 100));
    */


    //************************************************************************************************************************************





    //temporary calculating residue and atoms for plotting
    //residuum = signal;

        for(qint32 i = 0; i < myAtomList.length(); i++)
        {
            GaborAtom gaborAtom = myAtomList.at(i);
            residuum = gaborAtom.residuum;
            VectorXd bestMatch = gaborAtom.create_real(gaborAtom.sample_count, gaborAtom.scale, gaborAtom.translation, gaborAtom.modulation, gaborAtom.phase);//256, 20, 57, 95.505, PI);//

            //for(qint32 jj = 0; jj < gaborAtom.SampleCount; jj++)
            //    residuum(jj,0) -= gaborAtom.MaxScalarProduct * bestMatch[jj];

            VectorXd plotResiduum = VectorXd::Zero(t_iSize);

            for(qint32 ij = 0; ij < t_iSize; ij++)
            {
                plotResiduum[ij] = gaborAtom.residuum(ij,0);
            }

            QString title;          // string which will contain the result
            Plot *rPlot = new Plot(plotResiduum);
            title.append(QString("Resid: %1").arg(i));
            rPlot->setTitle(title);
            //rPlot->show();

            //find  maximum of Atom
            maximum = 0;
            for(qint32 ki = 1; ki < t_iSize; ki++)
                if(abs(maximum) < abs(gaborAtom.max_scalar_product * bestMatch[ki]))
                    maximum = gaborAtom.max_scalar_product * bestMatch[ki];
            std::cout << "hoechste Amplitude im Atom " << i << ":    " << maximum << "\n";

            //find  maximum of Residuum
            maximum = 0;
            for(qint32 mi = 1; mi < t_iSize; mi++)
                if(abs(maximum) < abs(residuum(mi,0)))
                    maximum = residuum(mi,0);
            std::cout << "hoechste Amplitude im Residuum " << i << ":    " << maximum << "\n";
            //delete gaborAtom;

        }

    //plot result of mp algorithm
    VectorXd approximation = VectorXd::Zero(signal.rows());

    for(qint32 i = 0; i < myAtomList.length(); i++)
    {
        GaborAtom gaborAtom = myAtomList.at(i);//new GaborAtom;
        //paintAtom = myAtomList.at(i);
        qint32 var1 = (gaborAtom.sample_count);
        qreal var2 = (gaborAtom.scale);
        qint32 var3 = (gaborAtom.translation);
        qreal var4 = gaborAtom.modulation;
        qreal var5 = gaborAtom.phase;
        //qreal var6 = gaborAtom.MaxScalarProduct;
        std::cout << "Parameter die Residuum bauen:\n   "<< " scale:  "  << var2 << " transl: " << var3 <<" modul: " << var4 <<" phase: " << var5 <<"\n";
        //std::cout << atan(1000000000)*180/PI << "\n";
        approximation += gaborAtom.max_scalar_product * gaborAtom.create_real(var1, var2, var3, var4, var5);

        QString title;          // string which will contain the title

        //plot atoms found
        VectorXd tmp = gaborAtom.create_real(var1, var2, var3, var4, var5);
        Plot *atPlot = new Plot(tmp);
        title.append(QString("Atom: %1").arg(i));
        atPlot->setTitle(title);
        //atPlot->show();
    }

    maximum = 0;
    for(qint32 i = 1; i < t_iSize; i++)
        if(abs(maximum) < abs(approximation[i]))
            maximum = approximation[i];
    std::cout << "hoechste Amplitude in Approximation ohne Residuum:    " << maximum << "\n";

    //plot approximation
    Plot *aPlot = new Plot(approximation);
    aPlot->setTitle("Approximation ohne Residuum");
    //aPlot->show();


    // in Graph malen
    _atomSumVector = approximation;
    update();


    for(qint32 i = 0; i < t_iSize; i++)
        approximation[i] += residuum(i,0);

    //ploat approxiamtion and residuum
    Plot *arPlot = new Plot(approximation);
    arPlot->setTitle("Approximation mit Residuum");
    //arPlot->show();

    //plot residuum
    VectorXd plotResiduum = VectorXd::Zero(t_iSize);
    for(qint32 i = 0; i < t_iSize; i++)
    {
        plotResiduum[i] = residuum(i,0);
    }

    // in Graph malen
    _residuumVector = plotResiduum;
    update();

    Plot *rPlot = new Plot(plotResiduum);
    rPlot->setTitle("Residuum");
    //rPlot->show();
}

void MainWindow::on_tbv_Results_cellClicked(int row, int column)
{
    QTableWidgetItem* firstItem = ui->tbv_Results->item(row, 0);
    GaborAtom atom = myAtomList.at(row);
    if(firstItem->checkState())
    {
        firstItem->setCheckState(Qt::Unchecked);
        _atomSumVector -= atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
        _residuumVector += atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
    }
    else
    {
        firstItem->setCheckState(Qt::Checked);
        _atomSumVector += atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
        _residuumVector -= atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
    }
    update();
}

/*
 * TODO: Calc MP (new)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
VectorXd MainWindow::mpCalc(QFile &currentDict, VectorXd signalSamples, qint32 iterationsCount)
{
    bool isDouble = false;

    qint32 atomCount = 0;
    qint32 bestCorrStartIndex;
    qreal sample;
    qreal bestCorrValue = 0;
    qreal residuumEnergie = 0;    

    QString contents;
    QString atomName;
    QString bestCorrName;

    QStringList bestAtom;
    QList<qreal> atomSamples;
    QList<QStringList> correlationList;
    VectorXd originalSignalSamples;
    VectorXd residuum;
    VectorXd bestCorrAtomSamples;
    VectorXd normBestCorrAtomSamples;

    residuum(0);
    bestCorrAtomSamples(0);
    normBestCorrAtomSamples(0);
    originalSignalSamples = signalSamples;    

    // Liest das Woerterbuch aus und gibt die Samples und den Namen an die Skalarfunktion weiter
    if (currentDict.open (QIODevice::ReadOnly))
    {
        while(!currentDict.atEnd())
        {
            contents = currentDict.readLine();
            if(contents.startsWith("atomcount"))
            {
                atomCount = contents.mid(12).toInt();
                break;
            }
        }
        qint32 i = 0;
        while(!currentDict.atEnd())
        {
            while(!currentDict.atEnd())
            {
                if(contents.contains("_ATOM_"))
                {
                    atomName = contents;
                    break;
                }
                contents = currentDict.readLine();
            }

            contents = "";
            while(!contents.contains("_ATOM_"))
            {
                contents = currentDict.readLine();
                sample = contents.toDouble(&isDouble);
                if(isDouble)
                    atomSamples.append(sample);
                if(currentDict.atEnd())
                    break;
            }
            correlationList.append(correlation(originalSignalSamples, atomSamples, atomName));

            atomSamples.clear();
        }
        currentDict.close();

        // Sucht aus allen verglichenen Atomen das beste passende herraus
        for(qint32 i = 0; i < correlationList.length(); i++)
        {
            if(fabs(correlationList.at(i).at(2).toDouble()) > fabs(bestCorrValue))
            {
                bestCorrName =  correlationList.at(i).at(0);
                bestCorrStartIndex = correlationList.at(i).at(1).toInt();
                bestCorrValue = correlationList.at(i).at(2).toDouble();              
            }
        }

        // Sucht das passende Atom im Woerterbuch und traegt des Werte in eine Liste
        if (currentDict.open (QIODevice::ReadOnly))
        {
            bool hasFound = false;
            qint32 j = 0;
            while(!currentDict.atEnd() )
            {
                contents = currentDict.readLine();
                if(QString::compare(contents, bestCorrName) == 0)
                {
                    contents = "";
                    while(!contents.contains("_ATOM_"))
                    {
                        contents = currentDict.readLine();
                        sample = contents.toDouble(&isDouble);
                        if(isDouble)
                        {
                            bestCorrAtomSamples[j] = sample;
                            j++;
                        }
                        if(currentDict.atEnd())
                            break;
                    }
                    hasFound = true;
                }
                if(hasFound) break;
            }
        }        
        currentDict.close();

        // Quadratische Normierung des Atoms auf den Betrag 1 und Multiplikation mit dem Skalarproduktkoeffizenten
        //**************************** Im Moment weil Testwoerterbuecher nicht nomiert ***********************************

        qreal normFacktorAtom = 0;
        for(qint32 i = 0; i < bestCorrAtomSamples.rows(); i++)
            normFacktorAtom += bestCorrAtomSamples[i]* bestCorrAtomSamples[i];
        normFacktorAtom = sqrt(normFacktorAtom);

        for(qint32 i = 0; i < bestCorrAtomSamples.rows(); i++)
            normBestCorrAtomSamples[i] = (bestCorrAtomSamples[i] / normFacktorAtom) * bestCorrValue;

        //**************************************************************************************************************

        // Subtraktion des Atoms vom Signal
        for(qint32 m = 0; m < normBestCorrAtomSamples.rows(); m++)
        {
            // TODO:
            //signalSamples.append(0);
            //signalSamples.prepend(0);
        }

        residuum = signalSamples;
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
        {
            residuum[normBestCorrAtomSamples.rows() + i + bestCorrStartIndex] = signalSamples[normBestCorrAtomSamples.rows() + i + bestCorrStartIndex] - normBestCorrAtomSamples[i];
            //residuum.removeAt(normBestCorrAtomSamples.rows() + i + bestCorrStartIndex + 1);
        }

        // Loescht die Nullen wieder
        for(qint32 j = 0; j < normBestCorrAtomSamples.rows(); j++)
        {
            // TODO:
            //residuum.removeAt(0);
            //residuum.removeAt(residuum.rows() - 1);
            //signalSamples.removeAt(0);
            //signalSamples.removeAt(signalSamples.rows() - 1);
        }

        iterationsCount++;

        // Traegt das gefunden Atom in eine Liste ein
        bestAtom.append(bestCorrName);
        bestAtom.append(QString("%1").arg(bestCorrStartIndex));
        bestAtom.append(QString("%1").arg(bestCorrValue));
        QString newSignalString = "";
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
            newSignalString.append(QString("%1/n").arg(normBestCorrAtomSamples[i]));

        bestAtom.append(newSignalString);
        globalResultAtomList.append(bestAtom);


        //***************** DEBUGGOUT **********************************************************************************
        QFile newSignal("Matching-Pursuit-Toolbox/newSignal.txt");
        if(!newSignal.exists())
        {
            if (newSignal.open(QIODevice::ReadWrite | QIODevice::Text))
            newSignal.close();
        }
        else    newSignal.remove();

        if(!newSignal.exists())
        {
            if (newSignal.open(QIODevice::ReadWrite | QIODevice::Text))
            newSignal.close();
        }

        if (newSignal.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &newSignal );
            for(qint32 i = 0; i < residuum.rows(); i++)
            {
                QString temp = QString("%1").arg(residuum[i]);
                stream << temp << "\n";
            }
        }
        newSignal.close();

        //**************************************************************************************************************


        // Eintragen und Zeichnen der Ergebnisss in die Liste der UI
        ui->tw_Results->setRowCount(iterationsCount);
        QTableWidgetItem* atomNameItem = new QTableWidgetItem(bestCorrName);

        // Berechnet die Energie des Atoms mit dem NormFaktor des Signals
        qreal normAtomEnergie = 0;
        for(qint32 i = 0; i < normBestCorrAtomSamples.rows(); i++)
            normAtomEnergie += normBestCorrAtomSamples[i] * normBestCorrAtomSamples[i];

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString("%1").arg(normAtomEnergie / signalEnergie * 100));

        //AtomWindow *atomWidget = new AtomWindow();
        //atomWidget->update();
        //ui->tw_Results->setItem(iterationsCount - 1, 1, atomWidget);
        //atomWidget->update();
        //QTableWidgetItem* atomItem = new QTableWidgetItem();
        //atomItem->

        ui->tw_Results->setItem(iterationsCount - 1, 0, atomNameItem);
        ui->tw_Results->setItem(iterationsCount - 1, 2, atomEnergieItem);


        ui->lb_IterationsProgressValue->setText(QString("%1").arg(iterationsCount));
        for(qint32 i = 0; i < residuum.rows(); i++)
            residuumEnergie += residuum[i] * residuum[i];
        if(residuumEnergie == 0)    ui->lb_RestEnergieResiduumValue->setText("0%");
        else    ui->lb_RestEnergieResiduumValue->setText(QString("%1%").arg(residuumEnergie / signalEnergie * 100));

        // Ueberprueft die Abbruchkriterien
        if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        {
            ui->progressBarCalc->setMaximum((1-sollEnergie)*100);
            processValue = (1 - residuumEnergie / signalEnergie + sollEnergie)*100;
            ui->progressBarCalc->setValue(processValue);
            if(ui->sb_Iterations->value() <= iterationsCount)
                ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

            if(ui->sb_Iterations->value() > iterationsCount && sollEnergie < residuumEnergie)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        else if(ui->chb_Iterations->isChecked())
        {
            ui->progressBarCalc->setMaximum(ui->sb_Iterations->value());
            processValue++;
            ui->progressBarCalc->setValue(processValue);

            if(ui->sb_Iterations->value() > iterationsCount)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
        else if(ui->chb_ResEnergy->isChecked())
        {
            ui->progressBarCalc->setMaximum((1-sollEnergie)*100);
            processValue = (1 - residuumEnergie / signalEnergie + sollEnergie)*100;
            ui->progressBarCalc->setValue(processValue);

            if(sollEnergie < residuumEnergie)
                residuum = mpCalc(currentDict, residuum, iterationsCount);
        }
    }
    return residuum;
}


// Berechnung das Skalarprodukt zwischen Atom und Signal
QStringList MainWindow::correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName)
{    
    qreal sum = 0;
    qint32 index = 0;
    qreal maximum = 0;
    qreal sumAtom = 0;

    VectorXd originalSignalList = signalSamples;
    QList<qreal> tempList;
    QList<qreal> scalarList;    
    QStringList resultList;

    resultList.clear();
    tempList.clear();

    // Quadratische Normierung des Atoms auf den Betrag 1
    //**************************** Im Moment weil Testwoerterbuecher nicht nomiert ***************************************

    for(qint32 i = 0; i < atomSamples.length(); i++)
        sumAtom += atomSamples.at(i)* atomSamples.at(i);
    sumAtom = sqrt(sumAtom);

    for(qint32 i = 0; i < atomSamples.length(); i++)
    {
        qreal tempVarAtom = atomSamples.at(i) / sumAtom;
        atomSamples.removeAt(i);
        atomSamples.insert(i, tempVarAtom);
    }

    // Fuellt das Signal vorne und hinten mit nullen auf damit Randwertproblem umgangen wird
    for(qint32 l = 0; l < atomSamples.length() - 1; l++)
    {
        //signalSamples.append(0);
        //signalSamples.prepend(0);
    }

    //******************************************************************************************************************

    for(qint32 j = 0; j < originalSignalList.rows() + atomSamples.length() -1; j++)
    {
        // Inners Produkt des Signalteils mit dem Atom
        for(qint32 g = 0; g < atomSamples.length(); g++)
        {
            tempList.append(signalSamples[g + j] * atomSamples.at(g));
            sum += tempList.at(g);
        }
        scalarList.append(sum);
        tempList.clear();
        sum = 0;
    }

    //Maximum und Index des Skalarproduktes finden unabhaengig ob positiv oder negativ
    for(qint32 k = 0; k < scalarList.length(); k++)
    {
        if(fabs(maximum) < fabs(scalarList.at(k)))
        {
            maximum = scalarList.at(k);
            index = k;
        }
    }

    // Liste mit dem Name des Atoms, Index und hoechster Korrelationskoeffizent
    resultList.append(atomName);
    resultList.append(QString("%1").arg(index -atomSamples.length() + 1));     // Gibt den Signalindex fuer den Startpunkt des Atoms wieder
    resultList.append(QString("%1").arg(maximum));

    return resultList;
    // die Stelle, an der die Korrelation am groessten ist ergibt sich aus:
    // dem Index des hoechsten Korrelationswertes minus die halbe Atomlaenge,

}
*/

// Opens Dictionaryeditor
void MainWindow::on_actionW_rterbucheditor_triggered()
{
    EditorWindow *x = new EditorWindow();
    x->show();
}

// opens advanced Dictionaryeditor
void MainWindow::on_actionErweiterter_W_rterbucheditor_triggered()
{
    Enhancededitorwindow *x = new Enhancededitorwindow();
    x->show();
}

// opens formula editor
void MainWindow::on_actionAtomformeleditor_triggered()
{
    Formulaeditor *x = new Formulaeditor();
    x->show();
}

// opens Filedialog for read signal (contextmenue)
void MainWindow::on_actionNeu_triggered()
{
    open_file();
}

// opens Filedialog for read signal (button)
void MainWindow::on_btt_OpenSignal_clicked()
{
    open_file();
}

//-----------------------------------------------------------------------------------------------------------------
//*****************************************************************************************************************


