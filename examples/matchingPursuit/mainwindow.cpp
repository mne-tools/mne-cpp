#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtGui"
#include "math.h"
#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"
#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>


qreal sollEnergie = 0;
qreal signalEnergie = 0;
qint32 valueLength;

QList<qreal> globalValueList;
QList<qreal> globalAtomList;
QList<qreal> globalResiduumList;

QList<QStringList> globalResultAtomList;
qint32 processValue = 0;


// Initialisierungsaufruf
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->progressBarCalc->setHidden(true);
    callGraphWindow = new GraphWindow();    
    callGraphWindow->setMinimumHeight(220);
    callGraphWindow->setMinimumWidth(500);

    callGraphWindow->setMaximumHeight(220);

    ui->sb_Iterations->setMaximum(9999);        // Legt das Maximum der Iterationsschritte fest
    ui->sb_Iterations->setMinimum(1);           // Legt das Minimum der Iterationsschritte fest

    atom = new Atom();

    ui->l_Graph->addWidget(callGraphWindow);
    ui->tb_ResEnergy->setText(tr("0,1"));
    // Erstellt die Konfigdatei bei Initalizierung
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

    // Ergebnisstabelle anpassen
    QStringList headerList;
    headerList << "Atomname" << "Atom" << "Energie (%)" << "Residuum";
    ui->tw_Results->setColumnCount(4);
    ui->tw_Results->setHorizontalHeaderLabels(headerList);
    QHeaderView *headerView = ui->tw_Results->horizontalHeader();
    headerView->setSectionResizeMode(0, QHeaderView::Interactive);
    headerView->setSectionResizeMode(1, QHeaderView::Stretch);
    headerView->setSectionResizeMode(2, QHeaderView::Interactive);
    headerView->setSectionResizeMode(3, QHeaderView::Stretch);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OpenFile()
{
    QFileDialog* fileDia;
    QString fileName = fileDia->getOpenFileName(this, "Bitte ein Signal auswählen",QDir::currentPath(),"Signal(*.txt)");
    if(fileName.isNull()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Fehler"),
        tr("Das Signal konnte nicht geöffnet werden."));
        return;
    }

    QString contents;
    globalValueList.clear();
    int rowNumber = 0;
    while(!file.atEnd())
    {
        contents = file.readLine(0).constData();

        bool isFloat;       
        qreal value = contents.toFloat(&isFloat);
        if(!isFloat)
        {
            QString errorSignal = QString("Das Signal konnte nicht vollständig interpretiert werden. Die Zeile %1 der Datei %2 konnte nicht gelesen werden.").arg(rowNumber).arg(fileName);
            QMessageBox::warning(this, tr("Fehler"),
            errorSignal);
            return;
        }       
        globalValueList.append(value);
        valueLength = globalValueList.length();
        rowNumber++;
    }

    file.close();
    signalEnergie = 0;
    for(qint32 i = 0; i < globalValueList.length(); i++)
        signalEnergie += (globalValueList.at(i) * globalValueList.at(i));
    update();
}

void GraphWindow::paintEvent(QPaintEvent* event)
{
    PaintSignal(globalValueList, globalResiduumList, Qt::red, this->size());
}

void GraphWindow::PaintSignal(QList<qreal> valueList, QList<qreal> residuumValues, QColor color, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(valueList.length() > 0)
    {
        qint32 borderMarginHeigth = 15;     // verkleinert Zeichenfläche von GraphWindow um borderMargin Pixel in der Höhe
        qint32 borderMarginWidth = 5;       // verkleinert Zeichenfläche von GraphWindow um borderMargin Pixel in der Breite
        qint32 i = 0;                       // Laufindex
        qreal maxNeg = 0;                   // Kleinster Signalwert
        qreal maxPos = 0;                   // groesster Signalwert
        qreal absMin = 0;                   // Minimum der Absolutbetraege von maxNeg und maxPos
        qint32 drawFactor = 0;              // Verschiebungsfaktor für Nachkommastellen (linear)
        qint32 startDrawFactor = 1;         // Verschiebungsfaktor für Nachkommastellen (exponentiell-Basis 10)
        qint32 decimalPlace = 0;            // Nachkommastellen für Achsenbeschriftung
        QPolygonF polygons;                 // Punkte zum Zeichnen des eingelesenen Signals
        QList<qreal> internListValue;       // interne representation der y-Werte des Signals (nur für grafische Darstellung)
        QPolygonF polygonsResiduum;

        internListValue.clear();
        while(i < valueList.length())
        {
                internListValue.append(valueList.at(i));            //TODO wie blöd
                i++;
        }

        // Fenster weiss übermalen
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // Maximum und Minimum des Signals finden
        i = 0;
        while(i < valueList.length())
        {
            if(valueList.at(i) > maxPos)
                maxPos = valueList.at(i);

            if(valueList.at(i) < maxNeg )
                maxNeg = valueList.at(i);
            i++;
        }

        if(maxPos > fabs(maxNeg)) absMin = maxNeg;       // findet das absolute Minimum der beiden globalen Extremwerte (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                 // absMin darf nicht null sein: sonst Endlosschleife
        {
            while(true)                                 // um wieviel muss die Nachkommastelle verschoben werden?
            {
                if(fabs(absMin) < 1)                     // Bei Signalen, bei denen absMin betragsmäßig größer 1 ist, muss keine Nachkommastelle verschoben werden
                {
                    absMin = absMin * 10;
                    drawFactor++;                       // Verschiebungfaktor (zählt die Anzahl der Nachkommastellen um die verschoben wurde
                }
                if(fabs(absMin) >= 1) break;
            }
        }

        // Verschiebung der Nachkommastellen um drawFactor für alle Signalpunkte und anschließende Übernahme in interne Liste
        while(drawFactor > 0)
        {
            i = 0;
            while(i < valueList.length())
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
        // Absolute Signalhöhe
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;


        // Achsenbeschriftung skalieren
        qreal scaleXText = (qreal)valueList.length() / (qreal)20;   //Signallänge teilen
        qreal scaleYText = (qreal)maxmax / (qreal)10;               // Signalwerte werden in 15tel unterteilt für y-Achsenbeschriftung (16 Werte)
        qint32 negScale =  round(maxNeg * 10 / maxmax);             // Startfaktor für y-Achsenbeschriftung

        //Bestimmen der Länge des Beschriftungstextes der y-Achse für Verschiebung der y-Achse nach rechts (damit Text nicht über Achse geschrieben wird)
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

        // Signal skalieren
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)valueList.length();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        //Achsen skalieren
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // Position der Achsbeschriftung der x-Achse
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // wenn Signal nur positiv: Beschriftung oberhalb der Achse

        i = 1;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = negScale * scaleYText / (qreal)startDrawFactor;                                    // Skalenwert Y-Achse
            string  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(negScale == 0)                                                                                       // x-Achse erreicht (y-Wert = 0)
            {
                // Eintragen der skalierten Signalpunkte
                qint32 h = 0;
                while(h < valueList.length())
                {
                    if(residuumValues.length() == valueList.length())
                        polygonsResiduum.append(QPointF((h * scaleX) + maxStrLenght,  -((residuumValues.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));

                    polygons.append(QPointF((h * scaleX) + maxStrLenght,  -((internListValue.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    h++;
                }

                // X-Achse zeichnen
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

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // Y-Achse zeichen

        painter.drawPolyline(polygons);                  // Signal zeichnen
        painter.setPen(color);
        if(residuumValues.length() == valueList.length())
        {
             painter.setPen(color);
            painter.drawPolyline(polygonsResiduum);         // Residuum zeichnen
        }
    }
    painter.end();    
}

// Erstellt die Atomwerte in eine Stringliste bei übergabe der Parameter
QStringList Atom::CreateStringValues(qint32 samples, qreal scale, qreal modulation, qreal phase, qreal chirp, Atom::AtomType atomType)
{
    QList<qreal> atomValues = Create(samples, scale, modulation, phase, chirp,atomType);
    QStringList atomStringValues;
    for(qint32 i = 0; i < atomValues.length(); i++)
        atomStringValues.append(QString("%1").arg(atomValues.at(i)));
    return atomStringValues;
}

// Erstellt die Atomwerte in eine Liste bei übergabe der Parameter
QList<qreal> Atom::Create(qint32 samples, qreal scale, qreal modulation, qreal phase, qreal chirp, Atom::AtomType atomType)
{
    int i = 0;
    QList<qreal> atomValues;


    atomValues.clear();

    if(atomType == Atom::Gauss)
    {
        if(scale != 0 && samples != 0)
        {
            while(i < samples)
            {
                qreal normI = (i - (qreal)samples / 2) / (qreal)scale;
                qreal exponentGauss = -3.14159265 * normI * normI;
                qreal gauss = pow(2, 0.25) * exp(exponentGauss);
                qreal angle = 6.2831853 * modulation / samples * i + phase;

                qreal result = (1/ sqrt(scale)) * gauss *  cos(angle);

                atomValues.append(result);
                i++;
            }
        }        
    }
    else if(atomType == Atom::Chirp)
    {
        if(scale != 0 && samples != 0)
        {
            qint32 normChirp = 0;
            if(chirp < 0)      normChirp = samples;

            i = 0;
            while( i < samples)
            {
                qreal normI = (i - (qreal)samples / 2) / (qreal)scale;
                qreal exponentGauss = -3.14159265 * normI * normI;
                qreal gauss = pow(2, 0.25) * exp(exponentGauss);
                qreal angle = (6.2831853 * modulation / samples) * i + (chirp / 2) * pow((i - normChirp), 2)  + phase;
                qreal result = (1/ sqrt(scale)) * gauss *  cos(angle);
                atomValues.append(result);
                i++;
            }
        }       
    }

    return atomValues;    
}

// Startet MP-algorithmus
void MainWindow::on_btt_Calc_clicked()
{
    processValue = 0;
    ui->progressBarCalc->setValue(0);
    ui->progressBarCalc->setHidden(false);

    if(globalValueList.isEmpty())
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
        QString text = "Es wurde kein Abbruchkriterium gewählt.";
        QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
        msgBox.exec();
        return;
    }

    if(((ui->tb_ResEnergy->text().toFloat() <= 1 && ui->tb_ResEnergy->isEnabled()) && (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled())) || (ui->tb_ResEnergy->text().toFloat() <= 1 && ui->tb_ResEnergy->isEnabled() && !ui->sb_Iterations->isEnabled()) || (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled() && !ui->tb_ResEnergy->isEnabled()) )
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

        qreal prozentValue = ui->tb_ResEnergy->text().toFloat(&ok);
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
        if(prozentValue >= 100)
        {
            QString title = "Fehler";
            QString text = "Bitte geben eine Zahl kleiner als 100% ein.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            ui->tb_ResEnergy->setFocus();
            ui->tb_ResEnergy->selectAll();
            return;
        }
        sollEnergie =  signalEnergie / 100 * prozentValue;
    }

    if(ui->rb_StandardDictionary->isChecked())
    {        
        QFile stdDict("Matching-Pursuit-Toolbox/stdDictionary.dict");
        globalResiduumList =  mpCalc(stdDict, globalValueList, 0);
        update();
    }
    else if(ui->rb_OwnDictionary->isChecked())
    {

        QFile ownDict(QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
        globalResiduumList =  mpCalc(ownDict, globalValueList, 0);
        update();
    }
    else if(ui->rb_TreebasedDictionary->isChecked())
    {
        // TODO
    }
}


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
QList<qreal> MainWindow::mpCalc(QFile &currentDict, QList<qreal> signalSamples, qint32 iterationsCount)
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
    QList<qreal> originalSignalSamples;
    QList<qreal> residuum;
    QList<qreal> bestCorrAtomSamples;
    QList<qreal> normBestCorrAtomSamples;

    residuum.clear();
    bestCorrAtomSamples.clear();
    normBestCorrAtomSamples.clear();
    originalSignalSamples = signalSamples;    

    // Liest das Wörterbuch aus und gibt die Samples und den Namen an die Skalarfunktion weiter
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

        // Sucht das passende Atom im Wörterbuch und trägt des Werte in eine Liste
        if (currentDict.open (QIODevice::ReadOnly))
        {
            bool hasFound = false;
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
                            bestCorrAtomSamples.append(sample);
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
        //**************************** Im Moment weil Testwörterbücher nicht nomiert ***********************************

        qreal normFacktorAtom = 0;
        for(qint32 i = 0; i < bestCorrAtomSamples.length(); i++)
            normFacktorAtom += bestCorrAtomSamples.at(i)* bestCorrAtomSamples.at(i);
        normFacktorAtom = sqrt(normFacktorAtom);

        for(qint32 i = 0; i < bestCorrAtomSamples.length(); i++)
            normBestCorrAtomSamples.append((bestCorrAtomSamples.at(i) / normFacktorAtom) * bestCorrValue);

        //**************************************************************************************************************

        // Subtraktion des Atoms vom Signal
        for(qint32 m = 0; m < normBestCorrAtomSamples.length(); m++)
        {
            signalSamples.append(0);
            signalSamples.prepend(0);
        }

        residuum = signalSamples;
        for(qint32 i = 0; i < normBestCorrAtomSamples.length(); i++)
        {
            residuum.insert(normBestCorrAtomSamples.length() + i + bestCorrStartIndex, signalSamples.at(normBestCorrAtomSamples.length() + i + bestCorrStartIndex) - normBestCorrAtomSamples.at(i));
            residuum.removeAt(normBestCorrAtomSamples.length() + i + bestCorrStartIndex + 1);
        }

        // Löscht die Nullen wieder
        for(qint32 j = 0; j < normBestCorrAtomSamples.length(); j++)
        {
            residuum.removeAt(0);
            residuum.removeAt(residuum.length() - 1);
            signalSamples.removeAt(0);
            signalSamples.removeAt(signalSamples.length() - 1);
        }

        iterationsCount++;

        // Trägt das gefunden Atom in eine Liste ein
        bestAtom.append(bestCorrName);
        bestAtom.append(QString("%1").arg(bestCorrStartIndex));
        bestAtom.append(QString("%1").arg(bestCorrValue));
        QString newSignalString = "";
        for(qint32 i = 0; i < normBestCorrAtomSamples.length(); i++)
            newSignalString.append(QString("%1/n").arg(normBestCorrAtomSamples.at(i)));

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
            for(qint32 i = 0; i < residuum.length(); i++)
            {
                QString temp = QString("%1").arg(residuum.at(i));
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
        for(qint32 i = 0; i < normBestCorrAtomSamples.length(); i++)
            normAtomEnergie += normBestCorrAtomSamples.at(i) * normBestCorrAtomSamples.at(i);

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
        for(qint32 i = 0; i < residuum.length(); i++)
            residuumEnergie += residuum.at(i) * residuum.at(i);
        if(residuumEnergie == 0)    ui->lb_RestEnergieResiduumValue->setText("0%");
        else    ui->lb_RestEnergieResiduumValue->setText(QString("%1%").arg(residuumEnergie / signalEnergie * 100));

        // Überprüft die Abbruchkriterien
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
QStringList MainWindow::correlation(QList<qreal> signalSamples, QList<qreal> atomSamples, QString atomName)
{    
    qreal sum = 0;
    qint32 index = 0;
    qreal maximum = 0;
    qreal sumAtom = 0;

    QList<qreal> originalSignalList = signalSamples;
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
        signalSamples.append(0);
        signalSamples.prepend(0);
    }

    //******************************************************************************************************************

    for(qint32 j = 0; j < originalSignalList.length() + atomSamples.length() -1; j++)
    {
        // Inners Produkt des Signalteils mit dem Atom
        for(qint32 g = 0; g < atomSamples.length(); g++)
        {
            tempList.append(signalSamples.at(g + j) * atomSamples.at(g));
            sum += tempList.at(g);
        }
        scalarList.append(sum);
        tempList.clear();
        sum = 0;
    }

    //Maximum und Index des Skalarproduktes finden unabhängig ob positiv oder negativ
    for(qint32 k = 0; k < scalarList.length(); k++)
    {
        if(fabs(maximum) < fabs(scalarList.at(k)))
        {
            maximum = scalarList.at(k);
            index = k;
        }
    }

    // Liste mit dem Name des Atoms, Index und höchster Korrelationskoeffizent
    resultList.append(atomName);
    resultList.append(QString("%1").arg(index -atomSamples.length() + 1));     // Gibt den Signalindex fuer den Startpunkt des Atoms wieder
    resultList.append(QString("%1").arg(maximum));

    return resultList;
    // die Stelle, an der die Korrelation am groessten ist ergibt sich aus:
    // dem Index des höchsten Korrelationswertes minus die halbe Atomlänge,

}

void AtomWindow::paintEvent(QPaintEvent* event)
{
    GraphWindow *win = new GraphWindow();
    win->PaintSignal(globalValueList, globalResiduumList, Qt::red, this->size());
}

/*void ResiduumWindow::paintEvent(QPaintEvent* event)
{
    //PaintSignal(globalValueList);
}*/

void MainWindow::on_btt_Close_clicked()
{
    close();
}

// Öffnet den Wörterbucheditor
void MainWindow::on_actionW_rterbucheditor_triggered()
{
    EditorWindow *x = new EditorWindow();
    x->show();
}

// Öffnet den Erweiterten Wörterbucheditor
void MainWindow::on_actionErweiterter_W_rterbucheditor_triggered()
{
    Enhancededitorwindow *x = new Enhancededitorwindow();
    x->show();
}

// Öffnet den Formeleditor
void MainWindow::on_actionAtomformeleditor_triggered()
{
    Formulaeditor *x = new Formulaeditor();
    x->show();
}

// Öffnet Filedialog zum Signal einlesen
void MainWindow::on_actionNeu_triggered()
{
    OpenFile();
}

// Öffnet Filedialog zum Signal einlesen
void MainWindow::on_btt_OpenSignal_clicked()
{
    OpenFile();
}
