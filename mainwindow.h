#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "notepad.h"
#include "config.h"
#include "searchdialog.h"
QT_BEGIN_NAMESPACE
QT_FORWARD_DECLARE_CLASS(QTabWidget)
QT_FORWARD_DECLARE_CLASS (QMenuBar)
QT_FORWARD_DECLARE_CLASS (QToolBar)
QT_FORWARD_DECLARE_CLASS (QFont)
QT_FORWARD_DECLARE_CLASS (QComboBox)
QT_FORWARD_DECLARE_CLASS (QFontComboBox)
QT_FORWARD_DECLARE_CLASS (QActionGroup)
QT_FORWARD_DECLARE_CLASS (QTextCharFormat)
QT_FORWARD_DECLARE_CLASS (QPrinter)
QT_END_NAMESPACE

#define EDITOR   static_cast<NotePad *>(tabWidget->currentWidget())

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Config *config, QWidget * = 0);
    ~MainWindow();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void moveEvent(QMoveEvent *event) override;

signals:

public slots:
    void currentChanged(int index); //tab发生改变时执行的槽 1
    void modificationChanged(bool changed); //文档发生改变 1
    void openFile();    //打开文件 1
    void openFile(QString FileName);    //打开文件 1
    void newFile(); //新建文件 1
    bool fileSaveAs(int index); //文件另存为（保存指定文件）1
    bool fileSave(int index);   //保存文件（保存指定文件）1
    bool fileSaveAs();  //文件另存为（保存当前文件）1
    bool fileSave();    //保存文件（保存当前文件）1
    bool fileSaveAll(); //保存所有文件 1
    void fileClose(int index);   //关闭文件（指定文件） 1
    void fileClose();   //关闭文件（当前文件） 1
    void fileCloseAll();    //关闭所有文件 1
    void nextWindow(); //下一个窗口 1
    void previousWindow(); //前一个窗口 1
    void currentAllWindow();    //当前所有窗口 1
    void setCurrentWindow(QAction *a);    //窗口菜单中的项被选中时触发的槽 1
    void openRecentFile();  //打开最近的文档 1
    void updateRecentFiles();    //更新最近打开的文件菜单 1
    void search();  //查找
    void about();   //关于本软件 1
private:
    void saveWindow();

    Config *config;//编辑器
    QTabWidget *tabWidget;//Tab栏
    SearchDialog *searchDialog; //查找/替换框
    int newNumber;//新建文件的数目
    QStringList openedFiles;//打开的文件
    QList<QAction * > recentFileActs;//最近打开的问文件
    QActionGroup *openedFilesGrp;//文件窗口Action Group


    QMenuBar *menuBar;//菜单栏
    QToolBar *topToolBar;   //第一个工具栏

    QMenu *fileMenu;    //文件菜单
    QAction *openAct;   //打开文件
    QAction *newAct;    //新建文件
    QAction *saveAct;   //保存文件
    QAction *saveAsAct;     //文件另存为
    QAction *saveAllAct;    //保存所有
    QAction *closeAct;  //关闭文件
    QAction *closeAllAct;   //关闭所有文件

    QMenu *editMenu;    //编辑菜单
    QAction *copyAct;   //复制
    QAction *cutAct;    //剪切
    QAction *pasteAct;  //粘贴
    QAction *undoAct;   //撤销
    QAction *redoAct;   //重做
    QAction *selectAllAct;  //全选
    QAction *findAct;   //查找和替换

    QMenu *compileMenu;//编译菜单
    QAction *function;//运行
    QAction *debug;//调试


    QMenu *windowMenu;  //窗口菜单
    QAction *nextAct;   //下一个窗口
    QAction *previousAct;   //上一个窗口
    QMenu *recentlyFilesMenu; //最近关闭的窗口
    QMenu *currentAllMenu;  //当前所有窗口

    QMenu *helpMenu;    //帮助菜单
    QAction *aboutAct;  //关于本软件
    QAction *aboutQtAct;    //关于Qt


    void init();    //初始化
    void closeEvent(QCloseEvent *);  //关闭事件
    void setupFileMenu();   //文件菜单功能实现
    void setupFileActions();    //文件菜单Action设置
    void setupEditMenu();   //编辑菜单功能实现
    void setupEditActions();    //编辑菜单Action设置
    void setupDebugMenu();    //调试菜单功能实现
    void setupDebugActions();  //调试菜单Action设置
    void setupWindowMenu(); //窗口菜单功能实现
    void fillRecentFileActs();  //填充recentFileActs
    void setupWindowActions();  //窗口菜单Action设置
    void setupHelpMenu();   //帮助菜单功能实现
    void setupHelpActions();    //帮助Action设置

    void newTab(const QString& fileName, QFile& file);  //创建新的Tab（用于打开文件）
    bool maybeSave(int index); //判断指定文件是否需要保存
    void closeDuplicate(int index); //关闭已经重复打开的文件
    void updateActions();   //更新各action的状态
    void refreshActions();  //更新action的状态(子函数)
    void updateRecentFilesList();    //更新config中最近打开的文件列表
    //void mergeFormatOnWordOrSelection(const QTextCharFormat &format); //设置文本格式（包括字体、颜色、风格）
    //void colorChanged(const QColor &col); //文本颜色发生改变
    //void fontChanged(const QFont &font); //字体发生改变
   // void updateComboStyle();   //更新ComboStyle下拉列表框的值
    //void currentCharFormatChanged(const QTextCharFormat &format); //文本格式发生改变
    void showReadme();  //显示readme文件

};

#endif // MAINWINDOW_H
