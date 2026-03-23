//=============================================================================================================
/**
 * @file     interfaces.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements shared interface constructors for MNE Analyze Studio.
 */

#include "ibuffer.h"
#include "imcpneuroskill.h"

using namespace MNEANALYZESTUDIO;

IBuffer::IBuffer(QObject* parent)
: QObject(parent)
{
}

IBuffer::~IBuffer() = default;

IMcpNeuroSkill::IMcpNeuroSkill(QObject* parent)
: QObject(parent)
{
}

IMcpNeuroSkill::~IMcpNeuroSkill() = default;
