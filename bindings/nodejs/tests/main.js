'use strict';

const electron = require ('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

var mainWindow = null;
var zyre = null;

var ipcMain = require('electron').ipcMain;

global.sharedObj = {prop1: "hello3"};

ipcMain.on('show-prop1', function(event) {
  console.log(global.sharedObj.prop1);
});


console.log(process.versions)

app.on ('window-all-closed', function () {
    if (process.platform != 'darwin') {
        app.quit ();
    }
});

app.on ('ready', function () {
    var ZyreBinding = require ('zyre');
    var zyre = new ZyreBinding.Zyre ();
    var zyrename = zyre.name ();
    console.log ('Node name is: ' + zyrename + ' EOL');
    global.sharedObj = {prop1: zyrename};

    mainWindow = new BrowserWindow ({ width:800, height:600 });
    mainWindow.loadURL ('file://' + __dirname + '/index.html');
    mainWindow.webContents.openDevTools ();

    mainWindow.on ('closed', function () {
        zyre.destroy ();
        mainWindow = null;
    });
});
