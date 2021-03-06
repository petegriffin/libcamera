/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * viewfinder.cpp - qcam - Viewfinder
 */

#include <QImage>
#include <QImageWriter>
#include <QMutexLocker>
#include <QPainter>

#include "format_converter.h"
#include "viewfinder.h"

ViewFinder::ViewFinder(QWidget *parent)
	: QWidget(parent), format_(0), width_(0), height_(0), image_(nullptr)
{
}

ViewFinder::~ViewFinder()
{
	delete image_;
}

void ViewFinder::display(const unsigned char *raw, size_t size)
{
	QMutexLocker locker(&mutex_);

	/*
	 * \todo We're not supposed to block the pipeline handler thread
	 * for long, implement a better way to save images without
	 * impacting performances.
	 */

	converter_.convert(raw, size, image_);
	update();
}

QImage ViewFinder::getCurrentImage()
{
	QMutexLocker locker(&mutex_);

	return image_->copy();
}

int ViewFinder::setFormat(unsigned int format, unsigned int width,
			  unsigned int height)
{
	int ret;

	ret = converter_.configure(format, width, height);
	if (ret < 0)
		return ret;

	format_ = format;
	width_ = width;
	height_ = height;

	delete image_;
	image_ = new QImage(width, height, QImage::Format_RGB32);

	updateGeometry();
	return 0;
}

void ViewFinder::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawImage(rect(), *image_, image_->rect());
}

QSize ViewFinder::sizeHint() const
{
	return image_ ? image_->size() : QSize(640, 480);
}
