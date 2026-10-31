// SPDX-License-Identifier: GPL-2.0-or-later
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QDateTime>
#include <QWindow>

class Probe : public QWidget {
public:
    Probe(QString name, QString logPath) : name(std::move(name)), log(std::move(logPath)) {
        const QMap<QString,QString> titles{{"A","Terminal"},{"B","File manager"},{"C","Web browser"},{"XW","Legacy application"}};
        displayTitle=titles.value(this->name,this->name);
        setWindowTitle(displayTitle);
        setAttribute(Qt::WA_AcceptTouchEvents);
        setMouseTracking(true);
        resize(720,720);
        if (!log.open(QIODevice::WriteOnly|QIODevice::Append)) qFatal("Cannot open probe log.");
    }
protected:
    void paintEvent(QPaintEvent *) override {
        if (!painted) { record("paint",mapToGlobal(QPoint(0,0))); painted=true; }
        QPainter p(this);p.fillRect(rect(),QColor("#172333"));
        p.setPen(QColor("#3f586e"));
        for (int x=0;x<width();x+=60)p.drawLine(x,0,x,height());
        for (int y=0;y<height();y+=60)p.drawLine(0,y,width(),y);
        p.setPen(QColor("#9de9d5"));p.setFont(QFont("sans",20));
        p.drawText(rect(),Qt::AlignCenter,displayTitle);
        p.setFont(QFont("sans",12));
        if(!lastEvent.isEmpty()) {
            p.setPen(QColor("#f18ac8"));p.drawEllipse(lastPoint,8,8);
            p.drawText(QRect(30,height()/2+25,width()-60,45),Qt::AlignCenter,
                QString("%1 (%2, %3)").arg(lastEvent).arg(lastPoint.x(),0,'f',1).arg(lastPoint.y(),0,'f',1));
            p.setPen(QColor("#9de9d5"));
        }
        p.drawText(15,30,"Menu: top-left | Dialog: top-right");
        p.drawText(15,height()-15,QString("%1 × %2").arg(width()).arg(height()));
        p.setPen(QPen(QColor("#ffcb77"),3));
        for (auto pos : {QPoint(30,60),QPoint(width()-30,60),QPoint(width()/2,height()/2),QPoint(30,height()-45),QPoint(width()-30,height()-45)}) {
            p.drawEllipse(pos,10,10);
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        record("mouse-down",e->position());
        if(QRectF(width()/2.-40,height()-75,80,30).contains(e->position()))
            record("system-move-request",e->position(),windowHandle()->startSystemMove()?1:0);
    }
    void mouseMoveEvent(QMouseEvent *e) override { record("mouse-move",e->position()); }
    void mouseReleaseEvent(QMouseEvent *e) override {
        record("mouse-up",e->position());
        if (e->position().y()<90 && e->position().x()<100) {
            auto menu=new QMenu(this);menu->setAttribute(Qt::WA_DeleteOnClose);
            menu->addAction("Test action",this,[this]{record("menu-action",{});});
            menu->popup(mapToGlobal(QPoint(20,80)));
        } else if (e->position().y()<90 && e->position().x()>width()-100) {
            auto dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setWindowTitle("Probe dialog");auto layout=new QVBoxLayout(dialog);
            layout->addWidget(new QLabel("Dialog interaction test",dialog));dialog->resize(280,180);dialog->show();
        }
    }
    bool event(QEvent *e) override {
        if(e->type()==QEvent::TouchBegin||e->type()==QEvent::TouchUpdate||e->type()==QEvent::TouchEnd||e->type()==QEvent::TouchCancel) {
            auto touch=static_cast<QTouchEvent*>(e);
            for(const auto &point:touch->points())record(QString("touch-%1").arg(int(e->type())),point.position(),point.id());
            e->accept();return true;
        }
        return QWidget::event(e);
    }
    void resizeEvent(QResizeEvent *e) override { QWidget::resizeEvent(e);record("resize",QPointF(width(),height())); }
private:
    void record(QString kind,QPointF pos,int id=-1) {
        QJsonObject event{{"window",name},{"event",kind},{"x",pos.x()},{"y",pos.y()},{"id",id},{"time",QDateTime::currentMSecsSinceEpoch()}};
        if(kind.startsWith("mouse-")||kind.startsWith("touch-")) {lastEvent=kind;lastPoint=pos;update();}
        const auto origin=mapToGlobal(QPoint(0,0));
        event.insert("originX",origin.x());event.insert("originY",origin.y());
        if(windowHandle())event.insert("frameTop",windowHandle()->frameMargins().top());
        log.write(QJsonDocument(event).toJson(QJsonDocument::Compact)+'\n');log.flush();
    }
    QString displayTitle,lastEvent;
    QPointF lastPoint;
    bool painted=false;
    QString name;
    QFile log;
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    const auto args=app.arguments();
    if(args.size()<3)return 2;
    app.setDesktopFileName("org.softyoda.LinuxRondProbe." + args[1]);
    Probe probe(args[1],args[2]);probe.show();return app.exec();
}
