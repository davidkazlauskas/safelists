
#include <gtkmm.h>
#include <templatious/FullPack.hpp>
#include <LuaPlumbing/plumbing.hpp>

struct GtkMainWindow : public Messageable {

    GtkMainWindow(Glib::RefPtr<Gtk::Builder>& bld) {
        Gtk::Window* outWnd = nullptr;
        bld->get_widget("applicationwindow1",outWnd);
        _wnd.reset( outWnd );
    }

    Gtk::Window& getWindow() {
        return *_wnd;
    }

    void message(templatious::VirtualPack& msg) override {
    }

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
    }

private:
    std::unique_ptr< Gtk::Window > _wnd;
};

int main(int argc,char** argv) {
    auto app = Gtk::Application::create(argc,argv);

    auto builder = Gtk::Builder::create();
    builder->add_from_file("uischemes/main.glade");
    auto mainWnd = std::make_shared< GtkMainWindow >(builder);
    app->run(mainWnd->getWindow(),argc,argv);
}

