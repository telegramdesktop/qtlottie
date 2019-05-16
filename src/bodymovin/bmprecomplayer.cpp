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
#include "bmprecomplayer.h"

#include "bmasset.h"
#include "bmbasictransform.h"
#include "bmscene.h"
#include "bmmasks.h"
#include "renderer.h"

namespace Lottie {

BMPreCompLayer::BMPreCompLayer(BMBase *parent) : BMLayer(parent) {
}

BMPreCompLayer::BMPreCompLayer(BMBase *parent, const BMPreCompLayer &other)
: BMLayer(parent, other) {
	if (other.m_layers) {
		m_layers = other.m_layers->clone(this);
	}
}

BMPreCompLayer::BMPreCompLayer(BMBase *parent, const JsonObject &definition)
: BMLayer(parent) {
	m_type = BM_LAYER_PRECOMP_IX;

	BMLayer::parse(definition);
	if (m_hidden) {
		return;
	}

	m_refId = definition.value("refId").toString();
}

BMPreCompLayer::~BMPreCompLayer() {
	if (m_layers) {
		delete m_layers;
	}
}

BMBase *BMPreCompLayer::clone(BMBase *parent) const {
	return new BMPreCompLayer(parent, *this);
}

void BMPreCompLayer::updateProperties(int frame) {
	if (m_updated) {
		return;
	}

	BMLayer::updateProperties(frame);

	const auto layersFrame = frame - m_startTime;
	if (m_layers && m_layers->active(layersFrame)) {
		m_layers->updateProperties(layersFrame);
	}
}

void BMPreCompLayer::render(Renderer &renderer, int frame) const {
	renderer.saveState();

	renderEffects(renderer, frame);

	renderer.render(*this);

	renderFullTransform(renderer, frame);

	if (m_masks) {
		m_masks->render(renderer, frame);
	}

	const auto layersFrame = frame - m_startTime;
	if (m_layers && m_layers->active(layersFrame)) {
		m_layers->render(renderer, layersFrame);
	}

	renderer.restoreState();
}

void BMPreCompLayer::resolveAssets(
		const std::function<BMAsset*(BMBase*, QByteArray)> &resolver) {
	if (m_layers) {
		return;
	}
	m_layers = resolver(this, m_refId);
	if (!m_layers) {
		qWarning()
			<< "BM PreComp Layer: asset not found: "
			<< QString::fromUtf8(m_refId);
	}
	BMLayer::resolveAssets(resolver);
}

} // namespace Lottie
