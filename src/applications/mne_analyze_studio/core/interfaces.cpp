//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     interfaces.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements shared interface constructors for MNE Analyze Studio.
 */

#include "ibuffer.h"
#include "iskilloperator.h"

using namespace MNEANALYZESTUDIO;

IBuffer::IBuffer(QObject* parent)
: QObject(parent)
{
}

IBuffer::~IBuffer() = default;

ISkillOperator::ISkillOperator(QObject* parent)
: QObject(parent)
{
}

ISkillOperator::~ISkillOperator() = default;
