/*!
 * COPYRIGHT (C) 2014 Emeric Grange - All Rights Reserved
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \file      mainwindow.cpp
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2014
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"

#include <iostream>

#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>

#include <QDropEvent>
#include <QUrl>
#include <QDebug>
#include <QMimeData>

#include <QTimer>

/* ************************************************************************** */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusLabel->hide();

    fcc = NULL;

    emptyFileList = true;

    statusTimer = new QTimer;
    connect(statusTimer, SIGNAL(timeout()), this, SLOT(hideStatus()));

    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(loadFileDialog()));
    connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(closeFile()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(About()));
    connect(ui->actionFourCC, SIGNAL(triggered()), this, SLOT(openFourCC()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), this, SLOT(AboutQt()));

    connect(ui->file_comboBox, SIGNAL(activated(int)), this, SLOT(printDatas()));

    // Save tabs titles and icons
    tabDropZoneText = ui->tabWidget->tabText(0);
    tabDropZoneIcon = ui->tabWidget->tabIcon(0);
    tabInfosText = ui->tabWidget->tabText(1);
    tabInfosIcon = ui->tabWidget->tabIcon(1);
    tabAudioText = ui->tabWidget->tabText(2);
    tabAudioIcon = ui->tabWidget->tabIcon(2);
    tabVideoText = ui->tabWidget->tabText(3);
    tabVideoIcon = ui->tabWidget->tabIcon(3);
    tabSubsText = ui->tabWidget->tabText(4);
    tabSubsIcon = ui->tabWidget->tabIcon(4);
    tabOtherText = ui->tabWidget->tabText(5);
    tabOtherIcon = ui->tabWidget->tabIcon(5);

    // "Drop zone" is the default tab when starting up
    handleTabWidget();

    // Accept video files "drag & drop"
    setAcceptDrops(true);

    // Auto-load a media file? (debug feature)
    //const QString file = "";
    //loadFile(file);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* ************************************************************************** */

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    // Check if the object dropped has an url (and is a file)
    if (e->mimeData()->hasUrls())
    {
        // TODO Filter by MimeType or file extension
        // Use QMimeDatabase // Qt 5.5

        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    foreach (const QUrl &url, e->mimeData()->urls())
    {
        const QString &fileName = url.toLocalFile();
        //std::cout << "Dropped file: "<< fileName.toStdString() << std::endl;

        loadFile(fileName);
    }
}

/* ************************************************************************** */

void MainWindow::loadFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open a multimedia file"),
                                                    "", tr("Files (*.*)"));

    std::cout << "Choosed file: "<< fileName.toStdString() << std::endl;

    loadFile(fileName);
}

int MainWindow::loadFile(const QString &file)
{
    int retcode = FAILURE;

    if (file.isEmpty() == false)
    {
        setStatus("Working...", SUCCESS, 0);

        retcode = analyseFile(file);

        if (retcode == 1)
        {
            handleComboBox(file);
            printDatas();
            hideStatus();
        }
        else
        {
            setStatus("The following file cannot be opened (UNKNOWN ERROR):\n'" + file + "'", FAILURE, 7500);
        }
    }

    return retcode;
}

/* ************************************************************************** */

VideoFile_t *MainWindow::currentMediaFile()
{
    VideoFile_t *media = NULL;

    size_t fileIndex = ui->file_comboBox->currentIndex();


    if (videosList.size() == 1)
    {
        media = videosList.at(0);
    }
    else if (fileIndex < videosList.size())
    {
        media = videosList.at(fileIndex);
    }

    return media;
}

void MainWindow::handleComboBox(const QString &file)
{
    // Is this the first file added?
    if (emptyFileList)
    {
        if (videosList.empty() == true)
        {
            ui->file_comboBox->addItem(QIcon(":/icons/icons/dialog-information.svg"), "Drag and drop files to analyse them!");
        }
        else
        {
            ui->file_comboBox->removeItem(0);
            emptyFileList = false;
        }
    }

    if (file.isEmpty() == false)
    {
        ui->file_comboBox->addItem(file);
        ui->file_comboBox->setCurrentIndex(ui->file_comboBox->count() - 1);

        //QIcon(":/icons/icons/audio-x-wav.svg")
        //QIcon(":/icons/icons/video-x-generic.svg")
    }
}

void MainWindow::handleTabWidget()
{
    ui->tabWidget->setEnabled(true);

    // Clean up all tabs
    while(ui->tabWidget->count() > 0)
    {
        ui->tabWidget->removeTab(0);
    }

    // Clean tabs
    if (videosList.empty())
    {
        // Add only the "drop zone" tab
        ui->tabWidget->addTab(ui->tab_dropzone, tabDropZoneIcon, tabDropZoneText);
        ui->tabWidget->tabBar()->hide();
        ui->tabWidget->setCurrentIndex(0);
    }
    else // Otherwise, adapt tabs to media file content
    {
        VideoFile_t *video = currentMediaFile();

        ui->tabWidget->tabBar()->show();
        ui->tabWidget->addTab(ui->tab_infos, tabInfosIcon, tabInfosText);

        if (video->tracks_video_count)
        {
            // Add the "video" tab
            ui->tabWidget->addTab(ui->tab_video, tabVideoIcon, tabVideoText);
        }

        if (video->tracks_audio_count)
        {
            // Add the "audio" tab
            ui->tabWidget->addTab(ui->tab_audio, tabAudioIcon, tabAudioText);
        }

        if (video->tracks_subtitles_count)
        {
            // Add the "subtitles" tab
            ui->tabWidget->addTab(ui->tab_subtitles, tabSubsIcon, tabSubsText);
        }

        if (video->tracks_others)
        {
            // Add the "others" tab
            ui->tabWidget->addTab(ui->tab_others, tabOtherIcon, tabOtherText);
        }
    }
}

/* ************************************************************************** */

void MainWindow::setStatus(const QString &text, int status, int duration)
{
    if (status == FAILURE)
    {
        ui->statusLabel->setStyleSheet("QLabel { border: 1px solid rgb(255, 53, 3);\nbackground: rgba(255, 170, 0, 128); }");
    }
    else //if (type == SUCCESS)
    {
        ui->statusLabel->setStyleSheet("QLabel { border: 1px solid rgb(85, 170, 0);\nbackground: rgba(85, 200, 0, 128); }");
    }

    if (duration > 0)
    {
        statusTimer->setInterval(7500);
        statusTimer->start();
    }

    ui->statusLabel->setText(text);
    ui->statusLabel->show();

    ui->statusLabel->repaint();
    qApp->processEvents();
}

void MainWindow::hideStatus()
{
    ui->statusLabel->hide();
}

void MainWindow::closeFile()
{
    // First clean the interface
    cleanDatas();

    // Then try to close the file's context
    int fileIndex = ui->file_comboBox->currentIndex();
    if (videosList.empty() == false)
    {
        if ((int)(videosList.size()) >= (fileIndex + 1))
        {
            videosList.erase(videosList.begin() + fileIndex);

            // Remove the entry from the comboBox
            ui->file_comboBox->removeItem(fileIndex);

            // Update comboBox index
            if (ui->file_comboBox->count() > 0)
            {
                ui->file_comboBox->activated(fileIndex-1);
            }
            else // No more file opened?
            {
                emptyFileList = true;
                QString empty;

                handleTabWidget();
                handleComboBox(empty);
            }
        }
    }
}

/* ************************************************************************** */

void MainWindow::About()
{
    QMessageBox about(QMessageBox::Information, tr("About mini_analyser"),
                      tr("<big><b>mini_analyser</b></big> \
                         <p align='justify'>mini_analyser is a software designed \
                         to help you extract the maximum of informations and meta-datas from multimedia files.</p> \
                         <p>This application is part of the MiniVideo framework.</p> \
                         <p>Emeric Grange (emeric.grange@gmail.com)</p>"),
                         QMessageBox::Ok);

    about.setIconPixmap(QPixmap(":/icons/icons/icon.svg"));
    about.exec();
}

void MainWindow::AboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

/* ************************************************************************** */

int MainWindow::analyseFile(const QString &file)
{
    int retcode = FAILURE;
    char input_filepath[4096];

    if (file.isEmpty() == false)
    {
        strcpy(input_filepath, file.toLocal8Bit());

        // Create and open the video file
        VideoFile_t *input_video = NULL;
        retcode = minivideo_open(input_filepath, &input_video);

        if (retcode == SUCCESS)
        {
            retcode = minivideo_parse(input_video, true, true, true);

            if (retcode == SUCCESS)
            {
                videosList.push_back(input_video);
            }
            else
            {
                std::cerr << "minivideo_parse() failed with retcode: " << retcode << std::endl;
            }
        }
        else
        {
            std::cerr << "minivideo_open() failed with retcode: " << retcode << std::endl;
        }
    }

    return retcode;
}

int MainWindow::printDatas()
{
    int retcode = 0;

    handleTabWidget();

    VideoFile_t *media = currentMediaFile();

    if (media)
    {
        ui->label_filename->setText(QString::fromLocal8Bit(media->file_name));
        ui->label_fullpath->setText(QString::fromLocal8Bit(media->file_path));
        ui->label_container->setText(getContainerString(media->container, 1));
        ui->label_container_extension->setText(QString::fromLocal8Bit(media->file_extension));
        ui->label_filesize->setText(getSizeString(media->file_size));
        ui->label_duration->setText(getDurationString(media->duration));

        if (media->container == CONTAINER_MP4)
        {
            QDate date(1904, 1, 1);
            QTime time(0, 0, 0, 0);
            QDateTime datetime(date, time);
            datetime = datetime.addSecs(media->creation_time);
            ui->label_creationdate->setText(datetime.toString("dddd d MMMM yyyy, hh:mm:ss"));
        }

        // AUDIO
        ////////////////////////////////////////////////////////////////////////

        int atid = 0;
        if (media->tracks_audio[atid] == NULL)
        {
            ui->groupBox_audio->hide();
        }
        else
        {
            ui->groupBox_audio->show();

            ui->label_audio_id->setText("0"); // stream id
            ui->label_audio_size->setText(getTrackSizeString(media->tracks_audio[atid], media->file_size));
            ui->label_audio_codec->setText(getCodecString(stream_AUDIO, media->tracks_audio[atid]->stream_codec));

            ui->label_audio_duration->setText(getDurationString(media->tracks_audio[atid]->duration));

            double bitrate = media->tracks_audio[atid]->bitrate;
            if (bitrate < 0.1)
            {
                for (unsigned i = 0; i < media->tracks_audio[atid]->sample_count; i++)
                    bitrate += media->tracks_audio[atid]->sample_size[i];
                bitrate /= media->tracks_audio[atid]->duration / 1000.0;
            }
            bitrate /= 1024.0;
            bitrate *= 8.0;

            ui->label_audio_bitrate->setText(QString::number(bitrate, 'g', 4) + " KB/s");
            if (media->tracks_audio[atid]->bitrate_mode == BITRATE_CBR)
                ui->label_audio_bitrate_mode->setText("CBR");
            else if (media->tracks_audio[atid]->bitrate_mode == BITRATE_VBR)
                ui->label_audio_bitrate_mode->setText("VBR");
            else if (media->tracks_audio[atid]->bitrate_mode == BITRATE_ABR)
                ui->label_audio_bitrate_mode->setText("ABR");
            else if (media->tracks_audio[atid]->bitrate_mode == BITRATE_CVBR)
                ui->label_audio_bitrate_mode->setText("CVBR");

            ui->label_audio_samplingrate->setText(QString::number(media->tracks_audio[atid]->sampling_rate));
            ui->label_audio_channels->setText(QString::number(media->tracks_audio[atid]->channel_count));
            ui->label_audio_bitpersample->setText(QString::number(media->tracks_audio[atid]->bit_per_sample));
        }

        // VIDEO
        ////////////////////////////////////////////////////////////////////////

        int vtid = 0;
        if (media->tracks_video[vtid] == NULL)
        {
            ui->groupBox_video->hide();
        }
        else
        {
            ui->groupBox_video->show();

            ui->label_video_id->setText("0"); // stream id
            ui->label_video_size->setText(getTrackSizeString(media->tracks_video[vtid], media->file_size));
            ui->label_video_codec->setText(getCodecString(stream_VIDEO, media->tracks_video[vtid]->stream_codec));

            ui->label_video_duration->setText(getDurationString(media->tracks_video[vtid]->duration));

            double bitrate = media->tracks_video[vtid]->bitrate;
            if (bitrate < 0.1)
            {
                for (unsigned i = 0; i < media->tracks_video[vtid]->sample_count; i++)
                    bitrate += media->tracks_video[vtid]->sample_size[i];
                bitrate /= media->tracks_video[vtid]->duration / 1000.0;
            }
            bitrate /= 1024.0;

            ui->label_video_bitrate->setText(QString::number(bitrate, 'g', 4) + " Kb/s");

            if (media->tracks_video[vtid]->bitrate_mode == BITRATE_CBR)
                ui->label_audio_bitrate_mode->setText("CBR");
            else if (media->tracks_video[vtid]->bitrate_mode == BITRATE_VBR)
                ui->label_audio_bitrate_mode->setText("VBR");
            else if (media->tracks_video[vtid]->bitrate_mode == BITRATE_ABR)
                ui->label_audio_bitrate_mode->setText("ABR");
            else if (media->tracks_video[vtid]->bitrate_mode == BITRATE_CVBR)
                ui->label_audio_bitrate_mode->setText("CVBR");

            ui->label_video_definition->setText(QString::number(media->tracks_video[vtid]->width) + " x " + QString::number(media->tracks_video[vtid]->height));
            ui->label_video_var->setText(getAspectRatioString(media->tracks_video[vtid]->width, media->tracks_video[vtid]->height));

            double framerate = media->tracks_video[vtid]->frame_rate;
            if (framerate < 0.1)
            {
                if (media->tracks_video[vtid]->duration && media->tracks_video[vtid]->sample_count)
                {
                    framerate = static_cast<double>(media->tracks_video[vtid]->sample_count / (static_cast<double>(media->tracks_video[vtid]->duration) / 1000.0));
                }
            }

            ui->label_video_framerate->setText(QString::number(framerate));
            ui->label_video_color_depth->setText(QString::number(media->tracks_video[vtid]->color_depth));
            ui->label_video_color_subsampling->setText(QString::number(media->tracks_video[vtid]->color_subsampling));
        }

        // SUBS
        ////////////////////////////////////////////////////////////////////////

        int stid = 0;
        if (media->tracks_subtitles[stid] != NULL)
        {
            // TODO ?
        }
    }
    else
    {
        retcode = 0;
    }

    return retcode;
}

void MainWindow::cleanDatas()
{
    //
}

void MainWindow::openFourCC()
{
    if (fcc)
    {
        fcc->show();
    }
    else
    {
        fcc = new FourccHelper();
        fcc->show();
    }
}

/* ************************************************************************** */
