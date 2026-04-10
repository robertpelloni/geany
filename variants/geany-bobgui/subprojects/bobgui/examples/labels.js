#!/usr/bin/env -S GI_TYPELIB_PATH=${PWD}/build/bobgui:${GI_TYPELIB_PATH} LD_PRELOAD=${LD_PRELOAD}:${PWD}/build/bobgui/libbobgui-4.so gjs

imports.gi.versions['Bobgui'] = '4.0';

const GObject = imports.gi.GObject;
const Bobgui = imports.gi.Bobgui;

const DemoWidget = GObject.registerClass({
  GTypeName: 'DemoWidget',
  }, class DemoWidget extends Bobgui.Widget {

  _init(params = {}) {
       super._init(params);

       let layout_manager = new Bobgui.GridLayout ();
       this.set_layout_manager (layout_manager);
       this.label1 = new Bobgui.Label({ label: "Red",
                                     hexpand: true,
                                     vexpand: true });
       this.label1.set_parent (this);
       let child1 = layout_manager.get_layout_child (this.label1);
       child1.set_row (0);
       child1.set_column (0);

       this.label2 = new Bobgui.Label({ label: "Green",
                                     hexpand: true,
                                     vexpand: true });
       this.label2.set_parent (this);
       let child2 = layout_manager.get_layout_child (this.label2);
       child2.set_row (0);
       child2.set_column (1);
    }
});

// Create a new application
let app = new Bobgui.Application({ application_id: 'org.bobgui.exampleapp' });

// When the application is launched…
app.connect('activate', () => {
    // … create a new window …
    let win = new Bobgui.ApplicationWindow({ application: app });
    // … with a button in it …
    let widget = new DemoWidget();
    win.set_child(widget);
    win.present();
});

// Run the application
app.run([]);

