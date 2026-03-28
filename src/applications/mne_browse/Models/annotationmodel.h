//=============================================================================================================
/**
 * @file     annotationmodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Table model for browser time-span annotations.
 */

#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QColor>
#include <QFile>
#include <QPair>
#include <QVector>

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

struct AnnotationSpanData {
    int     startSample = 0;
    int     endSample   = 0;
    QColor  color;
    QString label;
};

//=============================================================================================================
/**
 * DECLARE CLASS AnnotationModel
 */
class AnnotationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AnnotationModel(QObject *parent = nullptr);
    ~AnnotationModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    void setFiffInfo(const FIFFLIB::FiffInfo::SPtr& pFiffInfo);
    void setFirstLastSample(int firstSample, int lastSample);

    int addAnnotation(int startSample, int endSample, const QString& label);
    QPair<int, int> getSampleRange(int row) const;
    QVector<AnnotationSpanData> getAnnotationSpans() const;

    bool loadAnnotationData(QFile& qFile);
    bool saveAnnotationData(QFile& qFile) const;

    void clearModel();
    bool isFileLoaded() const;

signals:
    void annotationsChanged();

private:
    struct AnnotationEntry {
        int     startSample = 0;
        int     endSample   = 0;
        QString label;
    };

    AnnotationEntry normalizeEntry(int startSample, int endSample, const QString& label) const;
    void sortEntries();
    QColor colorForLabel(const QString& label) const;
    void notifyAnnotationsChanged();

    QVector<AnnotationEntry>  m_annotations;
    FIFFLIB::FiffInfo::SPtr   m_pFiffInfo;
    int                       m_iFirstSample = 0;
    int                       m_iLastSample  = 0;
    bool                      m_bFileLoaded  = false;
};

} // namespace MNEBROWSE

#endif // ANNOTATIONMODEL_H
