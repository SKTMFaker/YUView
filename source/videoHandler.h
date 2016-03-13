/*  YUView - YUV player with advanced analytics toolset
*   Copyright (C) 2015  Institut für Nachrichtentechnik
*                       RWTH Aachen University, GERMANY
*
*   YUView is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   YUView is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with YUView.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VIDEOHANDLER_H
#define VIDEOHANDLER_H

#include <QTreeWidgetItem>
#include <QDomElement>
#include <QDir>
#include <QString>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QThread>

#include "typedef.h"
#include "playlistItem.h"

#include "ui_videoHandler.h"

#include "videoCache.h"

#include <assert.h>

class videoCache;

/* TODO
*/
class videoHandler : public QObject, private Ui_videoHandler
{
  Q_OBJECT

public:

  /*
  */
  videoHandler();
  virtual ~videoHandler();

  virtual double getFrameRate() { return frameRate; }
  virtual QSize  getVideoSize() { return frameSize; }
  virtual indexRange getFrameIndexRange() { return startEndFrame; }

  virtual void drawFrame(QPainter *painter, int frameIdx, double zoomFactor);

  // caching --- different loading functions, depending on the type
  virtual bool loadIntoCache(int frameIdx) { Q_UNUSED(frameIdx); return true; }
  virtual bool isCaching();

  // The number of frames is needed for this class to update the start/end frame controls.
  // However, this class does no know how to calculate it. That depends on the file type/format.
  virtual unsigned int getNumberFrames() = 0;

  // Return the RGB values of the given pixel
  virtual ValuePairList getPixelValues(QPoint pixelPos);

  // Calculate the difference of this videoHandler to another videoHandler. This
  // function can be overloaded by more specialized video items. For example the videoHandlerYUV 
  // overloads this and calculates the difference directly on the YUV values (if possible).
  virtual QPixmap calculateDifference(videoHandler *item2, int frame, QList<infoItem> &differenceInfoList);
  // For the difference item: Return values of this item, the other item and the difference at
  // the given pixel position
  virtual ValuePairList getPixelValuesDifference(videoHandler *item2, QPoint pixelPos);
  
  // Every video item has a frameRate, start frame, end frame a sampling, a size and a number of frames
  double frameRate;
  indexRange startEndFrame;
  int sampling;
  QSize frameSize;

  // --- Caching: We have a cache object and a thread, where the cache object runs on
  videoCache *cache;
  QThread* cacheThread;

  // Create the video controls and return a pointer to the layout. This can be used by
  // inherited classes to create a properties widget.
  // isSizeFixed: For example a YUV file does not have a fixed size (the user can change this),
  // other sources might provide a fixed size which the user cannot change (HEVC file, png image sequences ...)
  virtual QLayout *createVideoHandlerControls(QWidget *parentWidget, bool isSizeFixed=false);

public slots:
  
  virtual void startCaching(indexRange range);
  virtual void stopCaching();
  void updateFrameCached() { emit signalHandlerChanged(false); }

signals:
  void signalHandlerChanged(bool redrawNeeded);

protected:

  // Set the values and update the controls. Only emit an event if emitSignal is set.
  void setFrameSize(QSize size, bool emitSignal = false);
  void setStartEndFrame(indexRange range, bool emitSignal = false);

  // --- Drawing: We keep a buffer of the current frame as RGB image so wen don't have to ´convert
  // it from the source every time a draw event is triggered. But if currentFrameIdx is not identical to
  // the requested frame in the draw event, we will have to update currentFrame.
  QPixmap    currentFrame;
  int        currentFrameIdx;

  // Draw the pixel values of the visible pixels in the center of each pixel.
  // Only draw values for the given range of pixels. 
  // The playlistItemVideo implememntation of this function will draw the RGB vales. However, if a derived class knows other 
  // source values to show it can overload this function (like the playlistItemYUVSource).
  virtual void drawPixelValues(QPainter *painter, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax, double zoomFactor);

  // Compute the MSE between the given char sources for numPixels bytes
  float computeMSE( unsigned char *ptr, unsigned char *ptr2, int numPixels ) const;

  // The video handler want's to draw a frame but it's not cached yet and has to be loaded.
  // The actual loading/conversion has to be performed by a specific video format handler implementation.
  // After this function was called, currentFrame should contain the requested frame and currentFrameIdx should
  // be equal to frameIndex.
  virtual void loadFrame(int frameIndex) = 0;

private:

  // A list of all frame size presets. Only used privately in this class. Defined in the .cpp file.
  class frameSizePresetList
  {
  public:
    // Constructor. Fill the names and sizes lists
    frameSizePresetList();
    // Get all presets in a displayable format ("Name (xxx,yyy)")
    QStringList getFormatedNames();
    // Return the index of a certain size (0 (Custom Size) if not found)
    int findSize(QSize size) { int idx = sizes.indexOf( size ); return (idx == -1) ? 0 : idx; }
    // Get the size with the given index.
    QSize getSize(int index) { return sizes[index]; }
  private:
    QList<QString> names;
    QList<QSize>   sizes;
  };

  // The (static) list of frame size presets (like CIF, QCIF, 4k ...)
  static frameSizePresetList presetFrameSizes;
  QStringList getFrameSizePresetNames();

  // We also keep a QImage version of the same frame for fast lookup of pixel values. If there is a look up and this
  // is not up to date, we update it.
  QRgb getPixelVal(QPoint pixelPos);
  QRgb getPixelVal(int x, int y);
  QImage     currentFrame_Image;
  int        currentFrame_Image_FrameIdx;

  bool controlsCreated;    ///< Have the video controls been created already?

private slots:
  // All the valueChanged() signals from the controls are connected here.
  void slotVideoControlChanged();
};

#endif // VIDEOHANDLER_H
