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
#include "bmlayer.h"

#include "bmshapelayer.h"
#include "bmfilleffect.h"
#include "bmbasictransform.h"

#include "bmscene.h"
#include "bmnulllayer.h"
#include "bmprecomplayer.h"
#include "bmmasks.h"
#include "bmmaskshape.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace Lottie {

BMLayer::BMLayer(BMBase *parent) : BMBase(parent), m_layerTransform(this) {
}

BMLayer::BMLayer(BMBase *parent, const BMLayer &other)
: BMBase(parent, other)
, m_layerIndex(other.m_layerIndex)
, m_startFrame(other.m_startFrame)
, m_endFrame(other.m_endFrame)
, m_startTime(other.m_startTime)
, m_blendMode(other.m_blendMode)
, m_3dLayer(other.m_3dLayer)
, m_stretch(other.m_stretch)
, m_layerTransform(this, other.m_layerTransform)
, m_parentLayer(other.m_parentLayer)
, m_td(other.m_td)
, m_clipMode(other.m_clipMode) {
	if (other.m_effects) {
		m_effects = new BMBase(this);
		for (BMBase *effect : other.m_effects->children()) {
			m_effects->appendChild(effect->clone(m_effects));
		}
	}
	if (other.m_masks) {
		m_masks = new BMMasks(this, *other.m_masks);
	}
	//m_transformAtFirstFrame = other.m_transformAtFirstFrame;
}

BMLayer::~BMLayer() {
	if (m_effects) {
		delete m_effects;
	}
	if (m_masks) {
		delete m_masks;
	}
}

BMLayer *BMLayer::construct(BMBase *parent, QJsonObject definition) {
	BMLayer *layer = nullptr;
	int type = definition.value(QStringLiteral("ty")).toInt();
	switch (type) {
	case 4:
		layer = new BMShapeLayer(parent, definition);
		break;
	case 3:
		layer = new BMNullLayer(parent, definition);
		break;
	case 0:
		layer = new BMPreCompLayer(parent, definition);
		break;
	default:
		qWarning() << "Unsupported layer type:" << type;
	}
	return layer;
}

bool BMLayer::active(int frame) const {
	return (!m_hidden && (frame >= m_startFrame && frame < m_endFrame));
}

void BMLayer::parse(const QJsonObject &definition) {
	BMBase::parse(definition);

	m_layerIndex = definition.value(QStringLiteral("ind")).toVariant().toInt();
	m_startFrame = definition.value(QStringLiteral("ip")).toVariant().toInt();
	m_endFrame = definition.value(QStringLiteral("op")).toVariant().toInt();
	m_startTime = definition.value(QStringLiteral("st")).toVariant().toReal();
	m_blendMode = definition.value(QStringLiteral("bm")).toVariant().toInt();
	m_autoOrient = definition.value(QStringLiteral("ao")).toBool();
	m_3dLayer = definition.value(QStringLiteral("ddd")).toBool();
	m_stretch = definition.value(QStringLiteral("sr")).toVariant().toReal();
	m_parentLayer = definition.value(QStringLiteral("parent")).toVariant().toInt();
	m_td = definition.value(QStringLiteral("td")).toInt();
	int clipMode = definition.value(QStringLiteral("tt")).toInt(-1);
	if (clipMode > -1 && clipMode < 5) {
		m_clipMode = static_cast<MatteClipMode>(clipMode);
	}

	const auto trans = definition.value(QStringLiteral("ks")).toObject();
	m_layerTransform.parse(trans);

	if (m_hidden) {
		return;
	}

	const auto maskProps = definition.value(QStringLiteral("masksProperties")).toArray();
	parseMasks(maskProps);

	const auto effects = definition.value(QStringLiteral("ef")).toArray();
	parseEffects(effects);

	if (m_td > 1) {
		qWarning()
			<< "BM Layer: Only alpha mask layer supported:" << m_clipMode;
	}
	if (m_blendMode > 0) {
		qWarning()
			<< "BM Layer: Unsupported blend mode" << m_blendMode;
	}
	if (m_stretch > 1) {
		qWarning()
			<< "BM Layer: stretch not supported" << m_stretch;
	}
	if (m_autoOrient) {
		qWarning()
			<< "BM Layer: auto-orient not supported";
	}
	if (m_3dLayer) {
		qWarning()
			<< "BM Layer: is a 3D layer, but not handled";
	}
}

void BMLayer::updateProperties(int frame) {
	if (m_updated) {
		return;
	}
	m_updated = true;

	if (m_parentLayer) {
		resolveLinkedLayer();
		if (m_linkedLayer) {
			m_linkedLayer->updateProperties(frame);
		}
	}

	// Update first effects, as they are not children of the layer
	if (m_effects) {
		for (BMBase* effect : m_effects->children()) {
			if (effect->active(frame)) {
				effect->updateProperties(frame);
			}
		}
	}

	if (m_masks) {
		m_masks->updateProperties(frame);
	}

	BMBase::updateProperties(frame);

	m_layerTransform.updateProperties(frame);
}

BMLayer *BMLayer::resolveLinkedLayer() {
	if (m_linkedLayer) {
		return m_linkedLayer;
	}

	for (BMBase *child : parent()->children()) {
		BMLayer *layer = static_cast<BMLayer*>(child);
		if (layer->layerId() == m_parentLayer) {
			m_linkedLayer = layer;
			break;
		}
	}
	return m_linkedLayer;
}

BMLayer *BMLayer::linkedLayer() const {
	return m_linkedLayer;
}

bool BMLayer::isClippedLayer() const {
	return m_clipMode != NoClip;
}

bool BMLayer::isMaskLayer() const {
	return m_td > 0;
}

BMLayer::MatteClipMode BMLayer::clipMode() const {
	return m_clipMode;
}

int BMLayer::layerId() const {
	return m_layerIndex;
}

void BMLayer::renderFullTransform(Renderer &renderer, int frame) const {
	// In case there is a linked layer, apply its transform first
	// as it affects tranforms of this layer too
	if (BMLayer *ll = linkedLayer()) {
		ll->renderFullTransform(renderer, frame);
	}
	m_layerTransform.render(renderer, frame);
}

void BMLayer::renderEffects(Renderer &renderer, int frame) const {
	if (!m_effects) {
		return;
	}
	for (BMBase* effect : m_effects->children())
		if (effect->active(frame)) {
			effect->render(renderer, frame);
		}
}

void BMLayer::parseEffects(const QJsonArray &definition, BMBase *effectRoot) {
	QJsonArray::const_iterator it = definition.constEnd();
	while (it != definition.constBegin()) {
		// Create effects container if at least one effect found
		if (!m_effects) {
			m_effects = new BMBase(this);
			effectRoot = m_effects;
		}
		it--;
		const auto effect = (*it).toObject();
		int type = effect.value(QStringLiteral("ty")).toInt();
		switch (type) {
		case 0: {
			BMBase *slider = new BMBase(effectRoot);
			slider->parse(effect);
			effectRoot->appendChild(slider);
		} break;

		case 5: {
			if (effect.value(QStringLiteral("en")).toInt()) {
				BMBase *group = new BMBase(effectRoot);
				group->parse(effect);
				effectRoot->appendChild(group);
				parseEffects(effect.value(QStringLiteral("ef")).toArray(), group);
			}
		} break;

		case 21: {
			BMFillEffect *fill = new BMFillEffect(effectRoot, effect);
			effectRoot->appendChild(fill);
		} break;

		default:
			qWarning()
				<< "BMLayer: Unsupported effect" << type;
		}
	}
}

void BMLayer::parseMasks(const QJsonArray &definition) {
	QJsonArray::const_iterator it = definition.constBegin();
	while (it != definition.constEnd()) {
		const auto mask = (*it).toObject();
		if (mask.value(QStringLiteral("mode")).toString() != QStringLiteral("n")) {
			if (!m_masks) {
				m_masks = new BMMasks(this);
			}
			m_masks->appendChild(new BMMaskShape(m_masks, mask));
		}
		++it;
	}
}

} // namespace Lottie
