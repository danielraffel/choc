//
//  CHOC example — native file drag-and-drop with choc::ui::WebView
//
//  Demonstrates both directions of file drag-and-drop between web content and
//  the OS, using two small additions to choc::ui::WebView:
//
//    • OUTBOUND (web -> OS): call window.chocStartFileDrag([paths]) from JS on
//      pointerdown/mousedown to begin a native drag of those files out of the
//      webview (e.g. into Finder / Explorer / a file manager).
//
//    • INBOUND (OS -> web): listen for the 'chocfilesdropped' CustomEvent; its
//      detail.paths is the array of OS file paths dropped onto the webview.
//
//  Build:
//    macOS : clang++ -std=c++17 -I.. webview_drag_and_drop.cpp -o dnd \
//                  -framework WebKit -framework Cocoa
//    Linux : g++ -std=c++17 -I.. webview_drag_and_drop.cpp -o dnd \
//                  $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1) -lpthread
//    Windows (MSVC): cl /EHsc /std:c++17 /I.. webview_drag_and_drop.cpp \
//                  /link ole32.lib oleaut32.lib shell32.lib user32.lib gdi32.lib
//    (or via examples/CMakeLists.txt: the `webview_drag_and_drop` target)
//
//  Platform notes:
//    • macOS (WKWebView) and Linux (WebKitGTK): both directions work.
//    • Windows (WebView2): inbound works. Outbound is not currently supported in
//      windowed WebView2 hosting (the render HWND owns mouse input, and the
//      DragStarting hook only exists on the composition controller). chocStartFileDrag
//      is still callable for API parity but is a no-op there.
//

#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#include "../choc/gui/choc_WebView.h"
#include "../choc/gui/choc_DesktopWindow.h"
#include "../choc/gui/choc_MessageLoop.h"
#include "../choc/text/choc_JSON.h"

int main()
{
    choc::messageloop::initialise();

    // Create a small file for the "drag out" demo to carry.
    auto exampleFile = (std::filesystem::temp_directory_path() / "choc_dnd_example.txt").string();
    { std::ofstream f (exampleFile); f << "Dragged out of a choc WebView.\n"; }
    std::cout << "Drag-out file: " << exampleFile << "\n";

    const auto html =
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><style>"
        "  body { font-family: -apple-system, system-ui, sans-serif; margin: 24px; color: #1d1d1f; }"
        "  .zone { width: 280px; height: 110px; margin-top: 14px; border: 2px dashed #5b8cff;"
        "          border-radius: 14px; display: flex; align-items: center; justify-content: center;"
        "          background: #eef4ff; user-select: none; font-weight: 600; text-align: center; }"
        "  #out { cursor: grab; } #in.hot { background: #dff5e1; border-color: #34a853; }"
        "  #log { margin-top: 16px; font: 12px ui-monospace, Menlo, Consolas, monospace; color: #555; white-space: pre-wrap; }"
        "</style></head><body>"
        "  <h2>choc WebView — file drag &amp; drop</h2>"
        "  <div id=\"out\" class=\"zone\">Drag me OUT to the desktop / a folder</div>"
        "  <div id=\"in\" class=\"zone\">Drop files here from the OS</div>"
        "  <div id=\"log\"></div>"
        "<script>"
        "  const file = " + choc::json::toString (choc::value::Value (exampleFile)) + ";"
        "  const out = document.getElementById('out'), drop = document.getElementById('in');"
        "  const log = (m) => { document.getElementById('log').textContent += m + '\\n'; };"
        "  // Arm the native drag-out on press (dragstart fires too late to catch the gesture)."
        "  const arm = () => { if (window.chocStartFileDrag) window.chocStartFileDrag([file]); };"
        "  out.addEventListener('pointerdown', arm);"
        "  out.addEventListener('mousedown', arm);"
        "  // Receive files dropped onto the webview from the OS."
        "  window.addEventListener('chocfilesdropped', (e) => {"
        "     drop.classList.add('hot');"
        "     log('Dropped in: ' + e.detail.paths.join(', '));"
        "     setTimeout(() => drop.classList.remove('hot'), 800);"
        "  });"
        "  log(window.chocStartFileDrag ? 'Ready.' : 'chocStartFileDrag unavailable on this platform.');"
        "</script></body></html>";

    choc::ui::DesktopWindow window ({ 100, 100, 520, 460 });
    window.setWindowTitle ("choc — Drag & Drop");

    choc::ui::WebView::Options options;
    options.enableDebugMode = true;

    // Set the page once the webview is ready (WebView2 initialises asynchronously).
    options.webviewIsReady = [&] (choc::ui::WebView& webview)
    {
        webview.setHTML (html);
    };

    auto webview = std::make_unique<choc::ui::WebView> (options);

    if (! webview->loadedOK())
    {
        std::cout << "Failed to create the WebView.\n";
        return 1;
    }

    window.setContent (webview->getViewHandle());
    window.setVisible (true);
    window.toFront();
    window.windowClosed = [] { choc::messageloop::stop(); };

    choc::messageloop::run();
    return 0;
}
