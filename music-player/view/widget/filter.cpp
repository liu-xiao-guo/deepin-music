/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "filter.h"

#include <QDebug>

#include <QTimer>
#include <QEvent>
#include <QCursor>
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsDropShadowEffect>
#include <QApplication>

#include <DUtil>

HoverFilter::HoverFilter(QObject *parent) : QObject(parent)
{

}

bool HoverFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Enter:
        QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
        return QObject::eventFilter(obj, event);;
    case QEvent::Leave:
        QApplication::restoreOverrideCursor();
        return QObject::eventFilter(obj, event);;
    default:
        return QObject::eventFilter(obj, event);
    }
}


class HintFilterPrivate
{
public:
    HintFilterPrivate(HintFilter *parent) : q_ptr(parent) {}

    void showHint(QWidget *hint);

    QTimer  *delayShowTimer = nullptr;

    QWidget *parentWidget = nullptr;
    QWidget *hintWidget = nullptr;

    HintFilter *q_ptr;
    Q_DECLARE_PUBLIC(HintFilter)
};


void HintFilterPrivate::showHint(QWidget *hint)
{
    if (!parentWidget) {
        return;
    }
    auto w = parentWidget;
    if (hintWidget && hintWidget != hint) {
        hintWidget->hide();
    }
    hintWidget = hint;
    if (!hintWidget) {
        return;
    }

    hintWidget->adjustSize();
    hintWidget->show();
    hintWidget->raise();

    DUtil::TimerSingleShot(10, [w, this]() {
        auto centerPos = w->mapToGlobal(w->rect().center());
        auto sz = hintWidget->size();
        centerPos.setX(centerPos.x()  - sz.width() / 2);
        centerPos.setY(centerPos.y() - 32 - sz.height());
        centerPos = hintWidget->mapFromGlobal(centerPos);
        centerPos = hintWidget->mapToParent(centerPos);
        hintWidget->move(centerPos);
    });
}

HintFilter::HintFilter(QObject *parent)  : QObject(parent), d_ptr(new HintFilterPrivate(this))
{
    Q_D(HintFilter);
    d->delayShowTimer = new QTimer;
    d->delayShowTimer->setInterval(1000);
    connect(d->delayShowTimer, &QTimer::timeout, this, [ = ]() {
        if (d->parentWidget) {
            auto hint = d->parentWidget->property("HintWidget").value<QWidget *>();
            d->showHint(hint);
        }
        d->delayShowTimer->stop();
    });
}

HintFilter::~HintFilter()
{

}

void HintFilter::hideAll()
{
    Q_D(HintFilter);
    if (d->hintWidget) {
        d->hintWidget->hide();
        d->delayShowTimer->stop();
    }
}

bool HintFilter::eventFilter(QObject *obj, QEvent *event)
{
    Q_D(HintFilter);
    switch (event->type()) {
    case QEvent::Enter: {
        if (d->hintWidget) {
            d->hintWidget->hide();
        }

        auto w = qobject_cast<QWidget *>(obj);
        d->parentWidget = w;
        if (!w) {
            break;
        }

        d->hintWidget = w->property("HintWidget").value<QWidget *>();
        if (!d->hintWidget) {
            break;
        }

        d->delayShowTimer->stop();

        bool nodelayshow = d->hintWidget->property("NoDelayShow").toBool();
        if (nodelayshow) {
            d->showHint(d->hintWidget);
        } else {
            d->delayShowTimer->start();
        }

        QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
        break;
    }
    case QEvent::Leave:
        if (d->hintWidget) {
            if (!d->hintWidget->property("DelayHide").toBool()) {
                d->hintWidget->hide();
                d->delayShowTimer->stop();
            } else {
                QMetaObject::invokeMethod(d->hintWidget, "deleyHide", Qt::DirectConnection);
            }
        }
        QApplication::restoreOverrideCursor();
        break;
    case QEvent::MouseButtonPress:
        if (d->hintWidget) {
            d->hintWidget->hide();
            d->delayShowTimer->stop();
        }

        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void HintFilter::showHitsFor(QWidget *w, QWidget *hint)
{
    Q_D(HintFilter);
    if (d->hintWidget) {
        d->hintWidget->hide();
    }

    d->parentWidget = w;
    if (!w) {
        return;
    }

    d->hintWidget = hint;
    if (!d->hintWidget) {
        return;
    }

    d->delayShowTimer->stop();

    d->showHint(hint);
    QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
}

HoverShadowFilter::HoverShadowFilter(QObject *parent): QObject(parent)
{

}

bool HoverShadowFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Enter: {
        auto w = qobject_cast<QWidget *>(obj);
        auto shadow = new QGraphicsDropShadowEffect(w);
        shadow->setBlurRadius(8);
        shadow->setOffset(0, 0);
        shadow->setColor(Qt::white);
        w->setGraphicsEffect(shadow);
        QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
        return QObject::eventFilter(obj, event);
    }
    case QEvent::Leave: {
        auto w = qobject_cast<QWidget *>(obj);
        w->graphicsEffect()->deleteLater();
        w->setGraphicsEffect(nullptr);
        QApplication::restoreOverrideCursor();
        return QObject::eventFilter(obj, event);
    }
    default:
        return QObject::eventFilter(obj, event);
    }
}
