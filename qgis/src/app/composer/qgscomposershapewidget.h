/***************************************************************************
                         qgscomposershapewidget.h
                         ------------------------
    begin                : November 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco@hugis.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSCOMPOSERSHAPEWIDGET_H
#define QGSCOMPOSERSHAPEWIDGET_H

#include "ui_qgscomposershapewidgetbase.h"

class QgsComposerShape;

/**Input widget for QgsComposerShape*/
class QgsComposerShapeWidget: public QWidget, private Ui::QgsComposerShapeWidgetBase
{
    Q_OBJECT
  public:
    QgsComposerShapeWidget( QgsComposerShape* composerShape );
    ~QgsComposerShapeWidget();

  private:
    QgsComposerShape* mComposerShape;

    /**Blocks / unblocks the signal of all GUI elements*/
    void blockAllSignals( bool block );
    /**Sets the GUI elements to the currentValues of mComposerShape*/
    void setGuiElementValues();

  private slots:
    void on_mShapeComboBox_currentIndexChanged( const QString& text );
    void on_mOutlineColorButton_clicked();
    void on_mOutlineWidthSpinBox_valueChanged( double d );
    void on_mTransparentCheckBox_stateChanged( int state );
    void on_mFillColorButton_clicked();
    void on_mRotationSpinBox_valueChanged( int val );
};

#endif // QGSCOMPOSERSHAPEWIDGET_H
