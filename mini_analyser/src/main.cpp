/*!
 * COPYRIGHT (C) 2018 Emeric Grange - All Rights Reserved
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
 * \file      main.cpp
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2014
 */

// mini_analyser
#include "main.h"

// minivideo library
#include <minivideo.h>

// C++ standard libraries
#include <iostream>
#include <cstdlib>
#include <cstring>

// Qt
#include <QtGlobal>
#include <QFileOpenEvent>

/* ************************************************************************** */

/*!
 * \brief Main (and only) function of this test software.
 * \param argc The number of argument contained in *argv[].
 * \param *argv[] A table containing program arguments.
 * \return 0 if everything went fine, the number of error(s) otherwise.
 *
 * This test software uses the minivideo library.
 */
int main(int argc, char *argv[])
{
    bool cli_enabled = false;
    bool cli_details_enabled = false;
    QStringList files;

    std::cout << GREEN "mini_analyser() arguments" RESET << std::endl;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i])
        {
            std::cout << "> " << argv[i] << std::endl;

            if (QString::fromLocal8Bit(argv[i]) == "--cli")
                cli_enabled = true;
            else if (QString::fromLocal8Bit(argv[i]) == "--details")
                cli_details_enabled = true;
            else
            {
                QString fileAsArgument = QFile::decodeName(argv[i]);
                if (QFile::exists(fileAsArgument))
                    files << fileAsArgument;
            }
        }
    }

    if (cli_enabled)
    {
        // CLI /////////////////////////////////////////////////////////////////

        MiniAnalyserCLI app(argc, argv);

        for (auto file: qAsConst(files))
        {
            app.cli.printFile(file, cli_details_enabled);
        }

        return EXIT_SUCCESS;
    }
    else
    {
        // GUI /////////////////////////////////////////////////////////////////

        std::cout << GREEN "mini_analyser(" BLUE << VERSION_STR << GREEN ")" RESET << std::endl;
#ifdef QT_DEBUG
        std::cout << "* DEBUG build" << std::endl;
#else
        std::cout << "* RELEASE build" << std::endl;
#endif
        std::cout << "* Qt version " << QT_VERSION_STR << std::endl;

        // Print informations about libMiniVideo and system endianness
        minivideo_print_infos();
        minivideo_print_features();
        minivideo_endianness();

#ifdef Q_OS_WIN32
        // High DPI monitor? Use automatic scaling
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

        // mini_analyser is a QApplication, with a mainwindow and which accepts QFileOpenEvent
        MiniAnalyserGUI app(argc, argv);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        // High DPI monitor? Use high quality pixmaps
        if (app.devicePixelRatio() > 1)
        {
            app.setAttribute(Qt::AA_UseHighDpiPixmaps);
        }
#endif

        // Launch program window
        app.gui.show();

        // Used to launch new detached instances
        if (argv[0])
        {
            app.gui.setAppPath(QString::fromLocal8Bit(argv[0]));
        }

        // If files have been passed as arguments, load them
        for (auto file: qAsConst(files))
        {
            app.gui.loadFile(file);
        }

        return app.exec();
    }

    return EXIT_FAILURE;
}

/* ************************************************************************** */

MiniAnalyserCLI::MiniAnalyserCLI(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    //
}

MiniAnalyserCLI::~MiniAnalyserCLI()
{
    //
}

/* ************************************************************************** */

MiniAnalyserGUI::MiniAnalyserGUI(int &argc, char **argv) : QApplication(argc, argv)
{
    //
}

MiniAnalyserGUI::~MiniAnalyserGUI()
{
    //
}

bool MiniAnalyserGUI::event(QEvent *e)
{
    if (e->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(e);
        if (QFile::exists(openEvent->file()))
        {
            gui.loadFile(openEvent->file());
        }
    }

    return QApplication::event(e);
}

/* ************************************************************************** */
