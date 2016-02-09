//  Hello World test for Zyre in Electron

'use strict';

const electron = require ('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

var mainWindow = null;
var zyre = null;

var ipcMain = require ('electron').ipcMain;

ipcMain.on ('hello', function (event) {
    console.log ("Hello, World");
});

app.on ('window-all-closed', function () {
    if (process.platform != 'darwin') {
        app.quit ();
    }
});

app.on ('ready', function () {
    console.log (process.versions);

    var ZyreBinding = require ('zyre');
    var zyre = new ZyreBinding.Zyre ();

    console.log ('Node name is: ' + zyre.name () + ' EOL');
    global.sharedObj = { zyre_name: zyre.name () };

    mainWindow = new BrowserWindow ({ width:800, height:600 });
    mainWindow.loadURL ('file://' + __dirname + '/index.html');

    mainWindow.on ('closed', function () {
        zyre.destroy ();
        mainWindow = null;
    });
});
