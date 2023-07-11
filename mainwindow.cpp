#include <QPrintDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QPrintEngine>
#include <QTextStream>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QTabWidget>
#include <QTextDocumentWriter>
#include <QMessageBox>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QToolBar>
#include <QFont>
#include <QClipboard>
#include <QComboBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QActionGroup>
#include <QTextCharFormat>
#include <QMimeData>
#include <QCoreApplication>
#include <QApplication>

#include "mainwindow.h"
#include "notepad.h"
#include "searchdialog.h"

MainWindow::MainWindow(Config *config,QWidget *parent)
    : QMainWindow(parent), config(config)
{
    init();
    setupFileMenu();   // 文件菜单功能实现
    setupEditMenu();   // 编辑菜单功能实现
    setupWindowMenu(); // 窗口菜单功能实现
    setupDebugMenu();  // 调试菜单功能实现
    setupHelpMenu();   // 帮助菜单功能实现

    currentChanged(-1);
    currentChanged(0);

    setupEditActions();   // 编辑菜单Action设置
    setupDebugActions();  // 调试菜单Action设置

    restoreGeometry(config->mainWindowsGeometry);
    restoreState(config->mainWindowState);
}

// 初始化
void MainWindow::init()
{
    menuBar = new QMenuBar(this);    // 菜单栏
    topToolBar = new QToolBar(this); // 第一个工具栏
    topToolBar->setObjectName("topToolBar");
    topToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setMenuBar(menuBar);
    addToolBar(topToolBar);
    addToolBarBreak(Qt::TopToolBarArea);

    tabWidget = new QTabWidget(this);
    tabWidget->setMovable(true);
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    tabWidget->setTabsClosable(true);
    connect(tabWidget,SIGNAL(currentChanged(int)),this,SLOT(currentChanged(int)));
    connect(tabWidget,SIGNAL(tabCloseRequested(int)),this,SLOT(fileClose(int)));
    setCentralWidget(tabWidget);

    searchDialog = new SearchDialog(config);
    searchDialog->setVisible(false);
}

void MainWindow::saveWindow()
{
    config->mainWindowsGeometry = saveGeometry();
    config->mainWindowState = saveState();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    this->saveWindow();
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    this->saveWindow();
}

void MainWindow::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowStateChange:
        this->saveWindow();
        break;
    default:
        break;
    }
}

// 判断指定文件是否需要保存 1
bool MainWindow::maybeSave(int index)
{
    NotePad *notePad = static_cast<NotePad*>(tabWidget->widget(index));
    QString fileName = openedFiles.at(index);
    if (!notePad->document()->isModified())      // 自定义一个警告对话框
        return true;
    if (fileName.startsWith(QLatin1String(":/"))) // startsWith判断该文件名是否是以什么开头的
        return true;
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Warning"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);//文档已被修改
    if (ret == QMessageBox::Save)
        return fileSave(index);
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

// Tab发生改变时触发的槽
void MainWindow::currentChanged(int index)
{
    if (index == -1) {
        newNumber = 0;
        if (config->showReadme) {
            showReadme();
        } else {
            if (!config->recentFiles.length()) {
                newFile();
            } else {
                QStringListIterator it(config->recentFiles);
                bool fileExists = false;
                while (it.hasNext()) {
                    QString fileName = it.next();
                    if (QFileInfo(fileName).exists()) {
                        openFile(fileName);
                        fileExists = true;
                    }
                }
                if (!fileExists) {
                    newFile();
                }
            }
        }
        //updateTextStyleActs(config->fontStyle);
        return;
    }
    updateActions();
    setWindowIcon(QIcon(tr(":images/notepad.png")));
    setWindowTitle(tr("Q-Text-Editor (%1)").arg(openedFiles.at(index)));
}

// 文档发生改变
void MainWindow::modificationChanged(bool changed)
{
    QString str = tabWidget->tabText(tabWidget->currentIndex());
    if (str[str.length() - 1] == '*') {
        if (!changed)
            str.resize(str.length() - 1);
    } else if (changed) {
        str += '*';
    }
    qDebug() << ">>> modificationChanged " << str;
    tabWidget->setTabText(tabWidget->currentIndex(), str);
    refreshActions();
    setupEditActions();
}

// 更新config中最近打开的文件列表
void MainWindow::updateRecentFilesList()
{
    int index = tabWidget->currentIndex();
    QString fileName = openedFiles.at(index);
    config->recentFiles.removeAll(fileName);
    config->recentFiles.prepend(fileName);
    if (config->recentFiles.size() > config->maxRecentFiles)
        config->recentFiles.removeLast();
}

void MainWindow::refreshActions()
{
    saveAct->setEnabled(EDITOR->document()->isModified());

    // copyAct->setEnabled(EDITOR->textCursor().hasSelection());
    connect(EDITOR, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));

    // cutAct->setEnabled(EDITOR->textCursor().hasSelection());
    connect(EDITOR, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));

    undoAct->setEnabled(EDITOR->document()->isUndoAvailable());
    redoAct->setEnabled(EDITOR->document()->isRedoAvailable());
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        pasteAct->setEnabled(md->hasText());
#endif
    nextAct->setEnabled(tabWidget->currentIndex()<tabWidget->count()-1);
    previousAct->setEnabled(tabWidget->currentIndex()>=1);
}

// 更新各Action的状态 1
void MainWindow::updateActions()
{
    connect(EDITOR, SIGNAL(modificationChanged(bool)), SLOT(modificationChanged(bool)), Qt::UniqueConnection);

    refreshActions();
    updateRecentFilesList();
}

// 关闭事件 1
void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < tabWidget->count(); i++)
    {
        tabWidget->setCurrentIndex(i);
        if (!maybeSave(i))
        {
            event->ignore();
            return;
        }
    }
    event->accept();
}

//关闭已经重复打开的文件 1
void MainWindow::closeDuplicate(int index)
{
    QString fileName = openedFiles.at(index);
    for (int i = 0; i < openedFiles.count(); i++) {
        if (openedFiles.at(i) == fileName && i != index) {
            openedFiles.removeAt(i);
            tabWidget->removeTab(i); // 去掉编号为i的tab
        }
    }
    int currIndex = openedFiles.indexOf(fileName); // 获得精准匹配的该位置的索引值
    tabWidget->setCurrentIndex(currIndex);
    setWindowTitle(tr("Q-Text-Editor (%1)").arg(fileName));
}

//创建新的Tab（用于打开文件）1
void MainWindow::newTab(const QString& fileName, QFile& file)
{
    openedFiles << fileName;//将该文件名加入文件列表中
    NotePad *notePad = new NotePad;
    tabWidget->addTab(notePad, QFileInfo(fileName).fileName());//QTabWidget，addTab 的作用是将notePad 添加到tab中去
    QByteArray data = file.readAll();
    QTextCodec* codec = QTextCodec::codecForName("utf-8");
    notePad->setPlainText(codec->toUnicode(data));  // 文本内容为这个
    tabWidget->setCurrentWidget(notePad);
}
//文件菜单功能实现
void MainWindow::setupFileMenu()
{
    fileMenu = new QMenu(tr("&File"), menuBar);

    //打开文件
    openAct = new QAction(QIcon(tr(":images/fileopen.png")), tr("&Open"), this);
    openAct->setShortcut(QKeySequence::Open);
    fileMenu->addAction(openAct);
    topToolBar->addAction(openAct);

    //新建文件
    newAct = new QAction(QIcon(tr(":images/filenew.png")), tr("&New"), this);
    newAct->setShortcut(QKeySequence::New);
    fileMenu->addAction(newAct);
    topToolBar->addAction(newAct);

    fileMenu->addSeparator();

    //保存文件
    saveAct = new QAction(QIcon(tr(":images/filesave.png")), tr("&Save"), this);
    saveAct->setShortcut(QKeySequence::Save);
    fileMenu->addAction(saveAct);
    topToolBar->addAction(saveAct);

    //文件另存为
    saveAsAct = new QAction(QIcon(tr(":images/filesaveas.png")),
                            tr("Save &As..."), this);
    fileMenu->addAction(saveAsAct);
    topToolBar->addAction(saveAsAct);

    //保存所有
    saveAllAct = new QAction(QIcon(tr(":images/saveall.png")), tr("Save All"), this);
    fileMenu->addAction(saveAllAct);
    topToolBar->addAction(saveAllAct);

    fileMenu->addSeparator();

    //关闭当前文件
    closeAct = new QAction(QIcon(tr(":images/fileclose.png")), tr("Close"),
                           this);
    closeAct->setShortcut(Qt::CTRL + Qt::Key_W);
    fileMenu->addAction(closeAct);
    topToolBar->addAction(closeAct);

    //关闭所有文件
    closeAllAct = new QAction(QIcon(tr(":images/closeall.png")),
                              tr("Close All"), this);
    closeAllAct->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_W);
    fileMenu->addAction(closeAllAct);
    topToolBar->addAction(closeAllAct);

    topToolBar->addSeparator();

    menuBar->addMenu(fileMenu);
    setupFileActions();

}
//文件菜单Action设置 1
void MainWindow::setupFileActions()
{
    connect(openAct, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(fileSave()));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    connect(saveAllAct, SIGNAL(triggered()), this, SLOT(fileSaveAll()));

    connect(closeAct, SIGNAL(triggered()), this, SLOT(fileClose()));
    connect(closeAllAct, SIGNAL(triggered()), this, SLOT(fileCloseAll()));
}
//打开文件 1
void MainWindow::openFile()
{
    QStringList files;
    files = QFileDialog::getOpenFileNames(this, tr("Open files..."), QString(),
                                          tr("All Files (*)"));
    QString fileName;
    if (files.count())
    {
        for (int i = 0; i < files.count(); i++)
        {
            fileName = files.at(i);
            if (QFile::exists(fileName))//判断文件是否存在
            {
                QFile file(fileName);
                if (file.open(QFile::ReadOnly))
                {
                    if (openedFiles.contains(fileName))
                        continue;
                    newTab(fileName, file);
                }
            }
        }
    }
}
//打开文件 1
void MainWindow::openFile(QString fileName)
{
    int index = openedFiles.indexOf(fileName);
    if (index != -1) {
        tabWidget->setCurrentIndex(index);
    } else {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly))
            newTab(fileName, file);
    }
}
//新建文件 1
void MainWindow::newFile()
{
    QString fileName = tr("New %1").arg(++newNumber);
    openedFiles << fileName;
    tabWidget->setCurrentIndex(tabWidget->addTab(new NotePad, fileName));
    // EDITOR->document()->setModified(true);
}
//文件另存为 1
bool MainWindow::fileSaveAs(int index)
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."), QString(),
                                              tr("All Files (*)"));
    if (fn.isEmpty())
        return false;

    openedFiles.replace(index, fn);//替换函数
    tabWidget->setTabText(index, QFileInfo(fn).fileName());
    return fileSave(index);
}

//保存文件 1
bool MainWindow::fileSave(int index)
{
    NotePad *notePad = static_cast<NotePad*>(tabWidget->widget(index));
    QString fileName = openedFiles.at(index);

    if (!fileName.contains("/") && !fileName.contains("\\"))
        return fileSaveAs(index);

    // 若要编写文档，请使用文件名或设备对象构造 QTextDocumentWriter 对象
    QTextDocumentWriter writer(fileName);
    writer.setFormat("plaintext");
    // 将给定的文档写入分配的设备或文件，如果成功，则返回1;否则返回0.
    bool success = writer.write(notePad->document());
    if (success) {
        notePad->document()->setModified(false); // setModified不可编辑功能
        tabWidget->setCurrentWidget(notePad);    // 获取当前页面
        setWindowTitle(tr("Q-Text-Editor (%1)").arg(fileName));
    } else {
        qDebug() << "fileSave error: " << fileName;
    }

    closeDuplicate(index);
    return success;
}

//文件另存为（保存当前文件）1
bool MainWindow::fileSaveAs()
{
    return fileSaveAs(tabWidget->currentIndex());
}

//保存文件（保存当前文件） 1
bool MainWindow::fileSave()
{
    return fileSave(tabWidget->currentIndex());
}

//保存所有文件 1
bool MainWindow::fileSaveAll()
{
    bool success = true;
    for (int i = 0; i < tabWidget->count(); i++)
    {
        tabWidget->setCurrentIndex(i);
        success = fileSave(i);
    }
    return success;
}

//关闭文件（指定文件）1
void MainWindow::fileClose(int index)
{
    if (maybeSave(index)) {
        if (openedFiles.count() == 1) {
            newFile();
            config->recentFiles.removeAll(openedFiles.at(0));
            openedFiles.removeAt(0);
            tabWidget->removeTab(0);
        } else {
            config->recentFiles.removeAll(openedFiles.at(index));
            openedFiles.removeAt(index);
            tabWidget->removeTab(index);
        }
    }
}

//关闭文件（当前文件）1
void MainWindow::fileClose()
{
    fileClose(tabWidget->currentIndex());
}

//关闭所有文件 1
void MainWindow::fileCloseAll()
{
    while (tabWidget->count() >= 1)
    {
        if (maybeSave(tabWidget->currentIndex()))
        {
            if (openedFiles.count() == 1)
            {
                newFile();
                openedFiles.removeAt(0);
                tabWidget->removeTab(0);
                break;
            }
            else
            {
                openedFiles.removeAt(tabWidget->currentIndex());
                tabWidget->removeTab(tabWidget->currentIndex());
            }
        }
        else
            break;
    }
}
//调试菜单功能实现
void MainWindow::setupDebugMenu()
{
    compileMenu = new QMenu(tr("&Debug"), menuBar);

    // 运行
    function = new QAction(QIcon(tr(":images/run.png")), tr("&Run"), this);
    function->setShortcut(Qt::CTRL + Qt::Key_F10);
    compileMenu->addAction(function);
    topToolBar->addAction(function);

    // 编译调试
    debug = new QAction(QIcon(tr(":images/debug.png")), tr("&deBug"), this);
    debug->setShortcut(Qt::CTRL  + Qt::Key_F9);
    compileMenu->addAction(debug);
    topToolBar->addAction(debug);
    topToolBar->addSeparator();
    menuBar->addMenu(compileMenu);
}
 //调试菜单Action设置
void MainWindow::setupDebugActions()
{

}
//编辑菜单功能实现 1
void MainWindow::setupEditMenu()
{
    editMenu = new QMenu(tr("&Edit"), menuBar);

    //复制
    copyAct = new QAction(QIcon(tr(":images/editcopy.png")), tr("&Copy"), this);
    copyAct->setShortcut(QKeySequence::Copy);
    editMenu->addAction(copyAct);
    topToolBar->addAction(copyAct);

    //剪切
    cutAct = new QAction(QIcon(tr(":images/editcut.png")), tr("&Cut"), this);
    cutAct->setShortcut(QKeySequence::Cut);
    editMenu->addAction(cutAct);
    topToolBar->addAction(cutAct);

    //粘贴
    pasteAct = new QAction(QIcon(tr(":images/editpaste.png")), tr("&Paste"), this);
    pasteAct->setShortcut(QKeySequence::Paste);
    editMenu->addAction(pasteAct);
    topToolBar->addAction(pasteAct);

    editMenu->addSeparator();

    //撤销
    undoAct = new QAction(QIcon(tr(":images/editundo.png")), tr("&Undo"), this);
    undoAct->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAct);
    topToolBar->addAction(undoAct);

    //重做
    redoAct = new QAction(QIcon(tr(":images/editredo.png")), tr("&Redo"), this);
    redoAct->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAct);
    topToolBar->addAction(redoAct);

    editMenu->addSeparator();

    //全选
    selectAllAct = new QAction(QIcon(tr(":images/editselectall.png")),
                               tr("&Select all"), this);
    selectAllAct->setShortcut(QKeySequence::SelectAll);
    editMenu->addAction(selectAllAct);
    topToolBar->addAction(selectAllAct);


    editMenu->addSeparator();
    //查找和替换
    findAct = new QAction(QIcon(tr(":images/editfind.png")), tr("&Find/Replace"),
                          this);
    findAct->setShortcut(QKeySequence::Find);
    editMenu->addAction(findAct);
    topToolBar->addAction(findAct);


    editMenu->addSeparator();

    menuBar->addMenu(editMenu);
}

//编辑菜单Action设置 1
void MainWindow::setupEditActions()
{
    connect(copyAct, SIGNAL(triggered()), EDITOR,SLOT(copy()));
    connect(pasteAct,SIGNAL(triggered()),EDITOR,SLOT(paste()));
    connect(undoAct,SIGNAL(triggered()),EDITOR,SLOT(undo()));
    connect(redoAct,SIGNAL(triggered()),EDITOR,SLOT(redo()));
    connect(selectAllAct,SIGNAL(triggered()),EDITOR,SLOT(selectAll()));
    connect(findAct,SIGNAL(triggered()),this,SLOT(search()));

}
//下一个窗口 1
void MainWindow::nextWindow()
{
    tabWidget->setCurrentIndex(tabWidget->currentIndex() + 1);
}

//前一个窗口 1
void MainWindow::previousWindow()
{
    tabWidget->setCurrentIndex(tabWidget->currentIndex() - 1);
}

//当前所有窗口 1
void MainWindow::currentAllWindow()
{
    int i;
    QAction *a;
    int index = tabWidget->currentIndex();

    //删除原先的Action
    foreach(a, openedFilesGrp->actions())
    {
        openedFilesGrp->removeAction(a);
        currentAllMenu->removeAction(a);
    }

    for (i = 0; i < tabWidget->count(); i++)
    {
        a = new QAction(tabWidget->tabText(i), this);
        a->setCheckable(true);
        if (i == index)
            a->setChecked(true);
        openedFilesGrp->addAction(a);
        currentAllMenu->addAction(a);
    }
    connect(openedFilesGrp, SIGNAL(triggered(QAction*)), this, SLOT(setCurrentWindow(QAction*)));
}

//窗口菜单中的项被选中时触发的槽 1
void MainWindow::setCurrentWindow(QAction *a)
{
    int index = openedFilesGrp->actions().indexOf(a);
    tabWidget->setCurrentIndex(index);
}

//打开最近的文档 1
void MainWindow::openRecentFile()
{
    openFile((static_cast<QAction*>(sender()))->data().toString());
}

//填充recentFileActs
void MainWindow::fillRecentFileActs()
{
    for (int i = 0; i < config->maxRecentFiles; i++) {
        QAction *act = recentlyFilesMenu->addAction("", this,
                                                    SLOT(openRecentFile()));
        act->setVisible(false);
        recentFileActs << act;
    }
}

//更新最近打开的文件菜单 1
void MainWindow::updateRecentFiles()
{
    int i = 0;
    QStringListIterator it(config->recentFiles);
    while (it.hasNext()) {
        QString fileName = it.next();
        if (QFileInfo(fileName).exists()) {
            recentFileActs[i]->setText(
                        tr("&%1: %2--%3").arg(i + 1).arg(QFileInfo(fileName).fileName()).arg(
                            fileName));
            recentFileActs[i]->setData(fileName);
            recentFileActs[i]->setVisible(true);
            i++;
        } else {
            config->recentFiles.removeAll(fileName);
        }
    }
}

//窗口菜单功能实现 1
void MainWindow::setupWindowMenu()
{
    windowMenu = new QMenu(tr("&Window"), menuBar);

    //下一个窗口
    nextAct = new QAction(QIcon(tr(":images/next.png")), tr("Next Window"), this);
    nextAct->setShortcut(Qt::CTRL + Qt::Key_Right);
    windowMenu->addAction(nextAct);
    topToolBar->addAction(nextAct);

    //上一个窗口
    previousAct = new QAction(QIcon(tr(":images/previous.png")),
                              tr("Previous Window"), this);
    previousAct->setShortcut(Qt::CTRL + Qt::Key_Left);
    windowMenu->addAction(previousAct);
    topToolBar->addAction(previousAct);

    //最近关闭的文件
    recentlyFilesMenu = new QMenu(tr("Recently Files"), windowMenu);
    fillRecentFileActs();
    windowMenu->addMenu(recentlyFilesMenu);

    //当前所有窗口
    currentAllMenu = new QMenu(tr("Current Windows"), windowMenu);
    windowMenu->addMenu(currentAllMenu);
    openedFilesGrp = new QActionGroup(this);

    topToolBar->addSeparator();
    menuBar->addMenu(windowMenu);
    setupWindowActions();
}

//窗口菜单Action设置 1
void MainWindow::setupWindowActions()
{
    connect(nextAct, SIGNAL(triggered()), SLOT(nextWindow()));
    connect(previousAct, SIGNAL(triggered()), SLOT(previousWindow()));
    connect(currentAllMenu, SIGNAL(aboutToShow()), SLOT(currentAllWindow()));
    connect(recentlyFilesMenu, SIGNAL(aboutToShow()), SLOT(updateRecentFiles()));
}

//帮助菜单功能实现 1
void MainWindow::setupHelpMenu()
{
    helpMenu = new QMenu(tr("&Help"), menuBar);

    //关于本软件
    aboutAct = new QAction(QIcon(tr(":images/helpabout.png")), tr("About"), this);
    helpMenu->addAction(aboutAct);
    topToolBar->addAction(aboutAct);

    //关于Qt
    aboutQtAct = new QAction(QIcon(tr(":images/helpaboutqt.png")), tr("About Qt"),
                             this);
    helpMenu->addAction(aboutQtAct);
    topToolBar->addAction(aboutQtAct);

    menuBar->addMenu(helpMenu);
    setupHelpActions();
}

//帮助Action设置 1
void MainWindow::setupHelpActions()
{
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAct, SIGNAL(triggered()),  (static_cast<QApplication *>(QCoreApplication::instance())), SLOT(aboutQt()));
}

//关于本软件 1
void MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates Qt's "
                                             "rich text editing facilities in action, providing an example "
                                             "document for you to experiment with."));
}


//显示readme文件 1
void MainWindow::showReadme()
{
    QString readmeFile = QApplication::applicationDirPath();
    readmeFile += readmeFile.endsWith('/') ? "readme.txt" : "/readme.txt";
    if (!QFile::exists(readmeFile))
    {
        QFile file(readmeFile);
        file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate);
        QTextStream stream(&file);
        stream
                << "/*===========================================================================\n"
                << "NotePad, editor for Common Use.\n"
                << "This program is free software: you can redistribute it and/or modify\n"
                << "it under the terms of the GNU General Public License as published by\n"
                << "the Free Software Foundation, either version 3 of the License, or\n"
                << "(at your option) any later version.\n" << "\n"
                << "This program is distributed in the hope that it will be useful,\n"
                << "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                << "GNU General Public License for more details.\n" << "\n"
                << "You should have received a copy of the GNU General Public License\n"
                << "along with this program.  If not, see <http://www.gnu.org/licenses/>\n"
                << "===========================================================================*/";
        file.close();
    }
    QFile rfile(readmeFile);
    rfile.open(QFile::ReadOnly);
    newTab(readmeFile, rfile);
}
//查找
void MainWindow::search()
{
    int index = tabWidget->currentIndex();
    tabWidget->setCurrentIndex(index);
    if (searchDialog->isVisible())
        searchDialog->activateWindow();
    else
        searchDialog->show();
    connect(searchDialog, SIGNAL(search(QString, bool, bool, bool)),EDITOR,SLOT(search(QString, bool, bool, bool)));
    connect(searchDialog, SIGNAL(replace(QString, QString, bool, bool, bool)), EDITOR,SLOT(replace(QString, QString, bool, bool, bool)));
    connect(searchDialog, SIGNAL(replaceAll(QString, QString, bool, bool)),EDITOR,SLOT(replaceAll(QString, QString, bool, bool)));
}

MainWindow::~MainWindow()
{
    // delete config;     // config配置

     delete tabWidget;      //Tab栏
     delete searchDialog;
     delete openedFilesGrp; // 文件窗口Action Grou
     delete menuBar;        // 菜单栏
     delete topToolBar;     // 第一个工具栏
     delete openAct;        // 打开文件
     delete newAct;         // 新建文件
     delete saveAct;        // 保存文件
     delete saveAsAct;      // 文件另存为
     delete saveAllAct;     // 保存所有
     delete closeAct;       // 关闭文件
     delete closeAllAct;    // 关闭所有文件

     delete cutAct;        // 剪切
     delete pasteAct;      // 粘贴
     delete undoAct;       // 撤销
     delete redoAct;       // 重做
     delete selectAllAct;  // 全选

     delete function; // 运行
     delete debug;    // 调试

     delete nextAct;       // 下一个窗口
     delete previousAct;   // 上一个窗口

     delete aboutAct;      // 关于本软件
     delete aboutQtAct;    // 关于Qt
}
