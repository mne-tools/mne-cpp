//=============================================================================================================
/**
 * @file     braintreemodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    BrainTreeModel class declaration.
 *
 */

#ifndef BRAINTREEMODEL_H
#define BRAINTREEMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QStandardItemModel>
#include <fs/surface.h>
#include <fs/annotation.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_sourcespace.h>
#include <fiff/fiff_dig_point.h>

namespace INVERSELIB {
class ECDSet;
}

class SurfaceTreeItem;
class BemTreeItem;

class BrainTreeModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit BrainTreeModel(QObject *parent = nullptr);
    ~BrainTreeModel() override = default;

    // Helper functions to populate the tree
    SurfaceTreeItem* addSurface(const QString &subject, const QString &hemi, const QString &surfType, const FSLIB::Surface &surface);
    
    // Add annotation to an existing surface item? Or separate item?
    // Usually annotation is property of a surface or child of surface.
    // Let's attach it to the surface item for now or find the surface item and update it.
    bool addAnnotation(const QString &subject, const QString &hemi, const FSLIB::Annotation &annotation);

    // Add BEM surface
    BemTreeItem* addBemSurface(const QString &subject, const QString &bemName, const MNELIB::MNEBemSurface &bemSurf);

    // Helpers to populate the tree
    void addSensors(const QString &type, const QList<QStandardItem*> &items);
    void addDipoles(const INVERSELIB::ECDSet &set);

    // Add digitizer points with proper categorization (Cardinal, HPI, EEG, Extra)
    void addDigitizerData(const QList<FIFFLIB::FiffDigPoint> &digitizerPoints);

    // Add source space points
    void addSourceSpace(const MNELIB::MNESourceSpace &srcSpace);

private:
   // Helpers to find specific items
   QStandardItem* getSubjectItem(const QString &subject);
};

#endif // BRAINTREEMODEL_H
