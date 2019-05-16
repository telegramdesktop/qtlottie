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
#include "bmtrimpath.h"

#include "trimpath.h"
#include "renderer.h"

#include <QtGlobal>
#include <private/qpainterpath_p.h>
#include <private/qbezier_p.h>

namespace Lottie {

BMTrimPath::BMTrimPath(BMBase *parent) : BMShape(parent) {
	m_appliedTrim = this;
}

BMTrimPath::BMTrimPath(BMBase *parent, const JsonObject &definition)
: BMShape(parent) {
	m_appliedTrim = this;
	parse(definition);
}

BMTrimPath::BMTrimPath(BMBase *parent, const BMTrimPath &other)
: BMShape(parent, other)
, m_start(other.m_start)
, m_end(other.m_end)
, m_offset(other.m_offset)
, m_simultaneous(other.m_simultaneous) {
}

BMBase *BMTrimPath::clone(BMBase *parent) const {
	return new BMTrimPath(parent, *this);
}

void BMTrimPath::parse(const JsonObject &definition) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	const auto start = definition.value("s").toObject();
	m_start.construct(start);

	const auto end = definition.value("e").toObject();
	m_end.construct(end);

	const auto offset = definition.value("o").toObject();
	m_offset.construct(offset);

	int simultaneous = true;
	if (definition.contains("m")) {
		simultaneous = definition.value("m").toInt();
	}
	m_simultaneous = (simultaneous == 1);

	if (strcmp(qgetenv("QLOTTIE_FORCE_TRIM_MODE"), "simultaneous") == 0) {
		m_simultaneous = true;
	} else if (strcmp(qgetenv("QLOTTIE_FORCE_TRIM_MODE"), "individual") == 0) {
		m_simultaneous = false;
	}
}

void BMTrimPath::updateProperties(int frame) {
	m_start.update(frame);
	m_end.update(frame);
	m_offset.update(frame);

	BMShape::updateProperties(frame);
}

void BMTrimPath::render(Renderer &renderer, int frame) const {
	if (m_appliedTrim) {
		if (m_appliedTrim->simultaneous()) {
			renderer.setTrimmingState(Renderer::Simultaneous);
		} else {
			renderer.setTrimmingState(Renderer::Individual);
		}
	} else {
		renderer.setTrimmingState(Renderer::Off);
	}

	renderer.render(*this);
}

bool BMTrimPath::acceptsTrim() const {
	return true;
}

void BMTrimPath::applyTrim(const BMTrimPath &other) {
	qreal newStart = other.start() + (m_start.value() / 100.0) *
			(other.end() - other.start());
	qreal newEnd = other.start() + (m_end.value() / 100.0) *
			(other.end() - other.start());

	m_start.setValue(newStart);
	m_end.setValue(newEnd);
	m_offset.setValue(m_offset.value() + other.offset());
}

qreal BMTrimPath::start() const {
	return m_start.value();
}

qreal BMTrimPath::end() const {
	return m_end.value();
}

qreal BMTrimPath::offset() const {
	return m_offset.value();
}

bool BMTrimPath::simultaneous() const {
	return m_simultaneous;
}

QPainterPath BMTrimPath::trim(const QPainterPath &path) const {
	TrimPath trimmer;
	trimmer.setPath(path);
	qreal offset = m_offset.value() / 360.0;
	qreal start = m_start.value() / 100.0;
	qreal end = m_end.value() / 100.0;
	QPainterPath trimmedPath;
	if (!qFuzzyIsNull(start - end)) {
		trimmedPath = trimmer.trimmed(start, end, offset);
	}
	return trimmedPath;
}

} // namespace Lottie
