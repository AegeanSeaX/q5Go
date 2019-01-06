/*
* qgo.cpp
*/

#include <QDir>
#include <QMessageBox>
#include <QLineEdit>
#include <QDebug>

#include "qgo.h"
#include "helpviewer.h"
#include "board.h"
#include "mainwindow.h"
#include "setting.h"
#include "defines.h"
#include "audio.h"

#include "config.h"
#include "searchpath.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

qGo::qGo() : QObject()
{
	helpViewer = NULL;
	clickSound = NULL;
	talkSound = NULL;
	matchSound = NULL;
	passSound = NULL;
	gameEndSound = NULL;
	timeSound = NULL;
	saySound = NULL;
	enterSound = NULL;
	leaveSound = NULL;
	connectSound = NULL;
	loadSound ();
}

qGo::~qGo()
{
	delete helpViewer;
	delete clickSound;
}

void qGo::quit()
{
	emit signal_leave_qgo();
	int check;
	if ((check = checkModified()) == 1 ||
		(!check &&
		!QMessageBox::warning(0, PACKAGE,
		tr("At least one board is modified.\n"
		"If you exit the application now, all changes will be lost!"
		"\nExit anyway?"),
		tr("Yes"), tr("No"), QString::null,
		1, 0)))
	{
		//	qApp->quit();
		qDebug() << "Program quits now...";
	}
	
	//    emit signal_leave_qgo();
}

void qGo::openManual()
{
	if (helpViewer == NULL)
		helpViewer = new HelpViewer(0);
	helpViewer->show();
	helpViewer->raise();
}


int qGo::checkModified()
{
	for (auto it: main_window_list) {
		if (!it->checkModified(false))
			return 0;
	}
	return 1;
}

void qGo::updateAllBoardSettings()
{
	for (auto it: main_window_list)
		it->updateBoard ();
}

void qGo::updateFont()
{
	for (auto it: main_window_list)
		it->updateFont ();

	emit signal_updateFont();
}

Sound * qGo::retrieveSound(const char * filename, SearchPath& sp)
{
  	QFile qfile(filename);
  	QFile * foundFile = sp.findFile(qfile);
	QString msg(filename);
  	if (! foundFile) 
	{
    		QString msg(filename);
    		msg.append(" not found");
    		qDebug() << msg;
    		return (Sound *) NULL;
  	}
	else 
	{
		msg.append(" found : " + foundFile->fileName());
		qDebug() << msg;
    		return SoundFactory::newSound(foundFile->fileName());
  	}
}

bool qGo::testSound(bool showmsg)
{
	// qDebug("qGo::testSound()");
	if (showmsg)
	{
		QMessageBox::information(0, PACKAGE, tr("Sound available."));
		return true;
	}
	
	//    qDebug("Sound available, checking for sound files...");

	// Sound files found?
	QStringList list;

	ASSERT(setting->program_dir);

#ifdef Q_WS_WIN
	list << applicationPath + "/sounds"
		<< setting->program_dir + "/sounds"
		<< "C:/Program Files/qGo/sounds"
		<< "D:/Program Files/qGo/sounds"
		<< "E:/Program Files/qGo/sounds"
		<< "C:/Programme/qGo/sounds"
		<< "D:/Programme/qGo/sounds"
		<< "E:/Programme/qGo/sounds"
		<< "./sounds";
#elif defined(Q_OS_MACX)
	//get the bundle path and find our resources like sounds
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	list << (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())		
		+ "/Contents/Resources";
#else
	// BUG 1165950 -- it may be better to use binreloc rather than
	// DATADIR
	list << DATADIR "/sounds"
	     << setting->program_dir + "/sounds"
		<< "./share/" PACKAGE "/sounds"
		<< "/usr/share/" PACKAGE "/sounds"
		<< "/usr/local/share/" PACKAGE "/sounds"
		<< "/sounds"
		<< "./sounds"
		<< "./src/sounds";                           //SL added eb 7
	
#endif

	SearchPath sp;
	sp << list;

	clickSound   = retrieveSound("stone.wav", sp);
	talkSound    = retrieveSound("talk.wav" , sp);
	matchSound   = retrieveSound("match.wav" , sp);
	passSound    = retrieveSound("pass.wav", sp);
	gameEndSound = retrieveSound("gameend.wav" , sp);
	timeSound    = retrieveSound("tictoc.wav" , sp);
	saySound     = retrieveSound("say.wav" , sp);
	enterSound   = retrieveSound("enter.wav", sp);
	leaveSound   = retrieveSound("leave.wav" , sp);
	connectSound = retrieveSound("connect.wav", sp);

#ifdef Q_WS_WIN
	if (soundsFound() && applicationPath.isEmpty())
	{
	  QFile qFile = QFile(connectSound->fileName()); // QQQ
	  QDir * dir = sp.findDirContainingFile(qFile); // QQQ
	  QString s = dir->dirName();
	  applicationPath = s.left(s.find("/sounds"));
	  // QMessageBox::information(0, "SAVING", applicationPath);
	}
			
#endif
	if (soundsFound())
	  return true;

#ifdef Q_OS_MACX
	QMessageBox::information(0, PACKAGE, tr("No sound files in bundle, strange.\n"));
#elif ! defined(Q_WS_WIN)
	QMessageBox::information(0, PACKAGE, tr("Sound files not found.") + "\n" +
		tr("Please check for the directories") + " /usr/local/share/" + PACKAGE + "/sounds/ " + tr("or") +
		" /usr/share/" + PACKAGE + "/sounds/, " + tr("depending on your installation."));
#else

	applicationPath = setting->readEntry("PATH_SOUND");
	if (applicationPath.isEmpty())
		return testSound(false);

	QMessageBox::information(0, PACKAGE, tr("Sound files not found.") + "\n" +
			     tr("You can navigate to the main qGo directory (for example:") + " C:\\Program Files\\" + PACKAGE + " .\n" +
				 tr("If the directory was given correctly, this data will be saved and you won't"
				 "be asked\nanymore except you install qGo again into a different directory.\n"
				 "To abort this procedure, click 'Cancel' in the following dialog."));

	applicationPath = QFileDialog::getExistingDirectory(NULL, NULL, "appdir", tr("qGo directory"), true);

	if (applicationPath.isNull() || applicationPath.isEmpty())
	{
		QMessageBox::warning(0, PACKAGE, tr("No valid directory was given. Sound is not available."));
		return false;
	}

	// save path
   	setting->writeEntry("PATH_SOUND", applicationPath);

	// QMessageBox::information(0, "TRYING AGAIN", applicationPath);
	return testSound(false);
#endif
	return false;
}

bool qGo::soundsFound()
{
  // success means all sounds were found
  if (clickSound && talkSound && matchSound && passSound
      && gameEndSound && timeSound && saySound && enterSound
      && leaveSound && connectSound)
    return true;
  else
    return false;
}

void qGo::playClick()
{
	if (clickSound) //setting->readBoolEntry("SOUND_STONE") && clickSound)
	{
		clickSound->play();
	}
}

void qGo::playAutoPlayClick()
{
	if (setting->readBoolEntry("SOUND_AUTOPLAY") && clickSound)
	{
		clickSound->play();
	}
}

void qGo::playTalkSound()
{
	if (setting->readBoolEntry("SOUND_TALK") && talkSound)
	{
		talkSound->play();
	}
}

void qGo::playMatchSound()
{
	if (setting->readBoolEntry("SOUND_MATCH") && matchSound)
	{
		matchSound->play();
	}
}

void qGo::playPassSound()
{
	if (setting->readBoolEntry("SOUND_PASS") && passSound)
	{
		passSound->play();
	}
}

void qGo::playGameEndSound()
{
	if (setting->readBoolEntry("SOUND_GAMEEND") && gameEndSound)
	{
		gameEndSound->play();
	}
}

void qGo::playTimeSound()
{
	if (setting->readBoolEntry("SOUND_TIME") && timeSound)
	{
		timeSound->play();
	}
}

void qGo::playSaySound()
{
	if (setting->readBoolEntry("SOUND_SAY") && saySound)
	{
		saySound->play();
	}
}

void qGo::playEnterSound()
{
	if (setting->readBoolEntry("SOUND_ENTER") && enterSound)
	{
		enterSound->play();
	}
}

void qGo::playLeaveSound()
{
	if (setting->readBoolEntry("SOUND_LEAVE") && leaveSound)
	{
		leaveSound->play();
	}
}

void qGo::playConnectSound()
{
	if (setting->readBoolEntry("SOUND_CONNECT") && connectSound)
	{
		connectSound->play();
	}
}

void qGo::playDisConnectSound()
{
	if (setting->readBoolEntry("SOUND_DISCONNECT") && connectSound)
	{
		connectSound->play();
	}
}

void qGo::slotHelpAbout()
{
	QString txt = PACKAGE " " VERSION
		"\n\nCopyright (c) 2001-2006\nPeter Strempel <pstrempel@t-online.de>\nJohannes Mesa <frosla@gmx.at>\nEmmanuel B�ranger <yfh2@hotmail.com>\n\n" +
		tr("GTP code from Goliath, thanks to:") + "\nPALM Thomas\nDINTILHAC Florian\nHIVERT Anthony\nPIOC Sebastien";
	
	QString translation = tr("English translation by:\nPeter Strempel\nJohannes Mesa\nEmmanuel Beranger", "Please set your own language and your name! Use your own language!");
	//if (translation != "English translation by:\nPeter Strempel\nJohannes Mesa\nEmmanuel B�ranger")
		txt += "\n\n" + translation;
	
	QMessageBox::about(0, tr("About..."), txt);
}
