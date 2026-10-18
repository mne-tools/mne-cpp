//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_implantation.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    sEEG / ECoG implantation: contacts grouped into depth leads, strips and grids.
 *
 * An implantation lists every contact with its position and groups the contacts into leads,
 * ordered along the lead (contact 1 first). The lead type decides how the contacts relate:
 * neighbours along a depth lead or strip, or rows and columns on a grid. Re-referencing and
 * display build on these neighbourhoods.
 */

#ifndef IEEG_IMPLANTATION_H
#define IEEG_IMPLANTATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE IEEGLIB
//=============================================================================================================

namespace IEEGLIB
{

//=============================================================================================================
/**
 * @brief Kind of lead a contact sits on.
 */
enum class IeegLeadType
{
    Depth, /**< Stereotactic depth lead (sEEG): contacts in a line inside the brain. */
    Strip, /**< Subdural strip (ECoG): one row of contacts on the cortex. */
    Grid   /**< Subdural grid (ECoG): rows x columns of contacts on the cortex. */
};

//=============================================================================================================
/**
 * @brief One recording contact.
 *
 * @snippet ex_ieeg/main.cpp ieeg_implantation_usage
 */
struct IEEGSHARED_EXPORT IeegContact
{
    QString name;                                       /**< Contact (channel) name, e.g. "LA3". */
    Eigen::Vector3f position = Eigen::Vector3f::Zero(); /**< Position in metres, in the frame of the implantation. */
    QString lead;                                       /**< Lead name; empty when building derives it from the contact name. */
};

//=============================================================================================================
/**
 * @brief One lead: a depth shaft, a strip or a grid.
 *
 * @snippet ex_ieeg/main.cpp ieeg_implantation_usage
 */
struct IEEGSHARED_EXPORT IeegLead
{
    QString name;                            /**< Lead name, e.g. "LA" or "GridA". */
    IeegLeadType type = IeegLeadType::Depth; /**< Lead type. */
    QVector<int> contacts;                   /**< Contact indices in order along the lead, contact 1 first. */
    int rows = 0;                            /**< Grid shape when known (rows x cols == contact count), else 0. */
    int cols = 0;                            /**< Grid shape when known, else 0. */
};

//=============================================================================================================
/**
 * @brief Contacts of an sEEG / ECoG implantation and the leads they belong to.
 *
 * @snippet ex_ieeg/main.cpp ieeg_implantation_usage
 */
class IEEGSHARED_EXPORT IeegImplantation
{
public:
    /** Leads whose closest neighbouring contacts are nearer than this are depth leads. */
    static constexpr float kMaxDepthSpacing = 0.006f;

    IeegImplantation() = default;

    //=========================================================================================================
    /**
     * Group contacts into leads.
     *
     * A contact without a lead name belongs to the lead named like the contact without its
     * trailing number (`LA1`, `LA10` -> `LA`; `A'1` stays apart from `A1`, the prime marking the
     * other hemisphere). Within a lead, contacts are ordered by that number. A lead whose closest
     * neighbouring contacts are less than @ref kMaxDepthSpacing apart is a depth lead, others
     * are strips.
     *
     * @param[in] contacts          Contacts, positions in metres.
     * @param[in] coordinateSystem  Name of the frame of the positions, e.g. "ACPC".
     */
    explicit IeegImplantation(const QVector<IeegContact>& contacts, const QString& coordinateSystem = QString());

    //=========================================================================================================
    /**
     * Read a BIDS iEEG `*_electrodes.tsv`.
     *
     * Units and the coordinate system come from the `*_coordsystem.json` in the same folder that
     * shares the longest name prefix; without one the file is refused, since the units are then
     * unknown. Contacts without coordinates are skipped. The `group` column assigns leads and
     * the `dimension` column ("[4x8]") the grid shape; a `type` naming depth, strip or grid sets
     * the lead type. Otherwise names and spacing decide, as in the constructor.
     *
     * @param[in]  electrodesTsv  Path to the *_electrodes.tsv file.
     * @param[out] error          Optional: why the file was refused.
     * @return The implantation; empty when the file was refused or has no placed contacts.
     */
    static IeegImplantation readBids(const QString& electrodesTsv, QString* error = nullptr);

    /** @return All contacts. */
    const QVector<IeegContact>& contacts() const
    {
        return m_contacts;
    }

    /** @return All leads, in order of their first contact. */
    const QVector<IeegLead>& leads() const
    {
        return m_leads;
    }

    /** @return Name of the frame of the positions, e.g. "ACPC"; empty when unknown. */
    QString coordinateSystem() const
    {
        return m_coordinateSystem;
    }

    /** @return True without contacts. */
    bool isEmpty() const
    {
        return m_contacts.isEmpty();
    }

    /**
     * @param[in] name  Contact name.
     * @return Index of the contact, or -1.
     */
    int indexOf(const QString& name) const;

    /** @return Contact positions, one row per contact, in metres. */
    Eigen::MatrixX3f positions() const;

private:
    QVector<IeegContact> m_contacts;
    QVector<IeegLead> m_leads;
    QString m_coordinateSystem;
};

} // namespace IEEGLIB

#endif // IEEG_IMPLANTATION_H
