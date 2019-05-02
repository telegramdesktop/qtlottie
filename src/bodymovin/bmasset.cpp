/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the lottie-qt module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bmasset_p.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>

#include "bmprecompasset_p.h"

QT_BEGIN_NAMESPACE

BMAsset *BMAsset::clone() const
{
    return new BMAsset(*this);
}

BMAsset *BMAsset::construct(QJsonObject definition)
{
    qCDebug(lcLottieQtBodymovinParser) << "BMAsset::construct()";

	BMAsset *asset = nullptr;
	if (definition.contains(QLatin1String("layers"))) {
		qCDebug(lcLottieQtBodymovinParser) << "Parse precomp asset";
		asset = new BMPreCompAsset(definition);
	}
    return asset;
}

void BMAsset::parse(const QJsonObject &definition)
{
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMAsset::parse():" << m_name;

    m_id = definition.value(QLatin1String("id")).toVariant().toString();
}

void BMAsset::resolveAssets(const std::function<BMAsset*(QString)> &resolver) {
	if (!m_resolved) {
		m_resolved = true;
		BMBase::resolveAssets(resolver);
	}
}

QString BMAsset::id() const
{
    return m_id;
}

QT_END_NAMESPACE
