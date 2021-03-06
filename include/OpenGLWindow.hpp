#pragma once

#include <functional>
#include <stdexcept>
#include <string>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #ifndef OGLW_NO_LIBS
        #pragma comment(lib, "opengl32.lib")
        #pragma comment(lib, "glu32.lib")
    #endif // !OGLW_NO_LIBS
#endif

#include <gl/GL.h>

namespace oglw {

    class WindowException : public virtual std::exception {
        std::string _what;
    public:
        WindowException(std::string const& what) : _what(what) { }
        const char* what() const override { return _what.c_str(); }
    };

    class WindowCreateException : public virtual WindowException {
        public: WindowCreateException(std::string const& what) : WindowException(what) { }
    };
    class WindowDestroyException : public virtual WindowException {
        public: WindowDestroyException(std::string const& what) : WindowException(what) { }
    };
    class WindowOpenGLException : public virtual WindowException {
        public: WindowOpenGLException(std::string const& what) : WindowException(what) { }
    };

    struct KeyInfo {
        KeyInfo(WPARAM wParam)
            : key(static_cast<unsigned>(wParam))
        {}
        unsigned key;
    };

    struct MouseInfo {
        enum class Button {
            None = 0, Left, Right, Middle,
        };

        int x, y;
        double normX, normY;
        Button button;
    };

    class OpenGLWindowBase
    {
    protected:
        bool isActive;
        unsigned sizeX, sizeY;

    public:
        std::function<void(void)> displayFunc;

        std::function<void(unsigned, unsigned)> resizeCallback;
        std::function<void(KeyInfo) > keydownCallback;
        std::function<void(KeyInfo) > keyupCallback;
        std::function<void(bool) > activateCallback;
        std::function<void(MouseInfo)> mousemoveCallback;
        std::function<void(MouseInfo)> mouseupCallback;
        std::function<void(MouseInfo)> mousedownCallback;

        bool active() const { return isActive; }
        unsigned getSizeX() const { return sizeX; }
        unsigned getSizeY() const { return sizeY; }

        OpenGLWindowBase(unsigned sizeX, unsigned sizeY)
            : sizeX(sizeX)
            , sizeY(sizeY)
        { }
    };

    struct OpenGLWindowParams {
        std::string const& title = "OpenGL window";
        unsigned width = 800;
        unsigned height = 600;
        unsigned char bits = 32;
        bool fullscreen = false;
    };
}

#ifdef _WIN32

namespace oglw {

    class WinAPIOGLWindow : public OpenGLWindowBase {
    protected:
        HDC m_hDC;            // Private GDI Device Context
        HGLRC m_hRC;            // Permanent Rendering Context
        HWND  m_hWnd;            // Holds Our Window Handle
        HINSTANCE m_hInstance;        // Holds The Instance Of The Application
        bool m_Fullscreen;
        bool throwOnGlErrors = true;

        static LRESULT CALLBACK WndProc(HWND    hWnd,            // Handle For This Window
            UINT    uMsg,            // Message For This Window
            WPARAM    wParam,            // Additional Message Information
            LPARAM    lParam)            // Additional Message Information
        {
            auto window = static_cast<WinAPIOGLWindow*>(reinterpret_cast<void*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)));

            switch (uMsg)                                    // Check For Windows Messages
            {
            case WM_CREATE:
                {
                    CREATESTRUCT *lpcs = reinterpret_cast<CREATESTRUCT*>(lParam);
                    WinAPIOGLWindow* win = reinterpret_cast<WinAPIOGLWindow*>(lpcs->lpCreateParams);

                    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
                }
                return 0;

            case WM_ACTIVATE:                            // Watch For Window Activate Message
                {
                    if (!HIWORD(wParam))                    // Check Minimization State
                        window->isActive = true;                        // Program Is Active
                    else
                        window->isActive = false;                        // Program Is No Longer Active

                    if (window->activateCallback)
                        window->activateCallback(window->isActive);

                    return 0;                                // Return To The Message Loop
                }

            case WM_SYSCOMMAND:                            // Intercept System Commands
                {
                    /*switch (wParam)                            // Check System Calls
                    {
                    case SC_SCREENSAVE:                    // Screensaver Trying To Start?
                    case SC_MONITORPOWER:                // Monitor Trying To Enter Powersave?
                    return 0;                            // Prevent From Happening
                    }*/
                    break;                                    // Exit
                }

            case WM_CLOSE:                                // Did We Receive A Close Message?
                {
                    PostQuitMessage(0);                        // Send A Quit Message
                    return 0;                                // Jump Back
                }

            case WM_KEYDOWN:                            // Is A Key Being Held Down?
                {
                    if (window->keyupCallback)
                        window->keyupCallback(KeyInfo { wParam });
                    return 0;
                }
            case WM_KEYUP:                                // Has A Key Been Released?
                {
                    if (window->keydownCallback)
                        window->keydownCallback(KeyInfo { wParam });
                    return 0;                                // Jump Back
                }

               
            case WM_LBUTTONDOWN:
                if (window->mousedownCallback){
                    window->mousedownCallback(mouseInfoFromMsgParam(wParam, lParam, window, MouseInfo::Button::Left));
                }
                return 0;
            case WM_RBUTTONDOWN:
                if (window->mousedownCallback){
                    window->mousedownCallback(mouseInfoFromMsgParam(wParam, lParam, window, MouseInfo::Button::Right));
                }
                return 0;
            case WM_MBUTTONDOWN:
                if (window->mousedownCallback){
                    window->mousedownCallback(mouseInfoFromMsgParam(wParam, lParam, window, MouseInfo::Button::Middle));
                }
                return 0;

            case WM_LBUTTONUP:
                if (window->mouseupCallback){
                    window->mouseupCallback(mouseInfoFromMsgParam(wParam, lParam, window, MouseInfo::Button::Left));
                }
                return 0;
            case WM_RBUTTONUP:
                if (window->mouseupCallback){
                    window->mouseupCallback(mouseInfoFromMsgParam(wParam, lParam, window, MouseInfo::Button::Right));
                }
                return 0;
            case WM_MBUTTONUP:
                if (window->mouseupCallback){
                    window->mouseupCallback(mouseInfoFromMsgParam(wParam, lParam, window, MouseInfo::Button::Middle));
                }
                return 0;

            case WM_MOUSEMOVE:
                {
                    if (window->mousemoveCallback){
                        window->mousemoveCallback(mouseInfoFromMsgParam(wParam, lParam, window));
                    }
                    return 0;
                }

            case WM_SIZE:                                // Resize The OpenGL Window
                {
                    unsigned Width = LOWORD(lParam);
                    unsigned Height = HIWORD(lParam);

                    window->sizeX = Width;
                    window->sizeY = Height;

                    if (window->resizeCallback)
                        window->resizeCallback(Width, Height);
                    return 0;                                // Jump Back
                }
            }

            // Pass All Unhandled Messages To DefWindowProc
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }

        void kill(void)                    // Properly kill The Window
        {
            try {
                if (m_Fullscreen) {
                    // Are We In Fullscreen Mode?
                    ChangeDisplaySettingsW(NULL, 0);                    // If So Switch Back To The Desktop
                    ShowCursor(TRUE);                                // Show Mouse Pointer
                }

                if (m_hRC) {
                    // Do We Have A Rendering Context?
                    if (!wglMakeCurrent(NULL, NULL)) {
                        m_hRC = nullptr;
                        throw WindowDestroyException("Release Of DC And RC Failed.");
                    }

                    if (!wglDeleteContext(m_hRC)) {
                        // Are We Able To Delete The RC?
                        m_hRC = nullptr;
                        throw WindowDestroyException("Release Rendering Context Failed.");
                    }
                }

                if (m_hDC && !ReleaseDC(m_hWnd, m_hDC)) {
                    // Are We Able To Release The DC
                    m_hDC = nullptr;
                    throw WindowDestroyException("Release Device Context Failed.");
                }

                if (m_hWnd && !DestroyWindow(m_hWnd)) {
                    m_hWnd = nullptr;
                    throw WindowDestroyException("Could Not Release hWnd.");
                }

                if (!UnregisterClassW(L"OpenGL", m_hInstance)) {
                    m_hInstance = nullptr;
                    throw WindowDestroyException("Could Not Unregister Class.");
                }
            }
            catch (WindowException &) {
                throw;
            }
        }

        static MouseInfo mouseInfoFromMsgParam(WPARAM wParam, LPARAM lParam, WinAPIOGLWindow* window, 
            MouseInfo::Button button = MouseInfo::Button::None) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            double normX = static_cast<double>(x) / window->sizeX;
            double normY = static_cast<double>(y) / window->sizeY;

            /*MouseInfo::Button b;
            switch (wParam) {
                case MK_LBUTTON: b = MouseInfo::Button::Left; break;
                case MK_RBUTTON: b = MouseInfo::Button::Right; break;
                case MK_MBUTTON: b = MouseInfo::Button::Middle; break;
                case 0: default:
                    b = MouseInfo::Button::None; break;
            }*/
            return MouseInfo { x, y, normX, normY, button };
        }

    public:
        void close() {
            PostQuitMessage(0);
        }

        void display() {
            if (displayFunc) {
                displayFunc();
            }

            if (throwOnGlErrors) {
                auto err = glGetError();
                if (err) {
                    throw WindowOpenGLException("OpenGL Error occured: " + std::to_string(err));
                }
            }

            SwapBuffers(m_hDC);
        }

        bool enableFullScreen(unsigned width, unsigned height, unsigned bits ) {
            DEVMODE dmScreenSettings;                                // Device Mode
            memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));    // Makes Sure Memory's Cleared
            dmScreenSettings.dmSize = sizeof(dmScreenSettings);        // Size Of The Devmode Structure
            dmScreenSettings.dmPelsWidth = width;                // Selected Screen Width
            dmScreenSettings.dmPelsHeight = height;                // Selected Screen Height
            dmScreenSettings.dmBitsPerPel = bits;                    // Selected Bits Per Pixel
            dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            // Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
            if (ChangeDisplaySettingsW(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                return false;
            }
            else
                return true;
        }

        bool process() {
            MSG msg;
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))    // Is There A Message Waiting?
            {
                if (msg.message == WM_QUIT) {
                    return false;
                }
                TranslateMessage(&msg);                // Translate The Message
                DispatchMessageW(&msg);                // Dispatch The Message
            }
            return true;
        }

        /*void signalErrorMessage(const std::string& message, const std::string& caption = std::string()) {
            MessageBox(m_hWnd, message.c_str(), caption.c_str(), MB_OK | MB_ICONEXCLAMATION);
            throw std::runtime_error(message);
        }*/

        WinAPIOGLWindow(
            OpenGLWindowParams const& parameters,
            std::function<HGLRC(HDC)> contextCreator = std::function<HGLRC(HDC)>()
            )
            : m_Fullscreen(parameters.fullscreen)
            , OpenGLWindowBase(parameters.width, parameters.height)
        {
            try {
                unsigned        PixelFormat;            // Holds The Results After Searching For A Match
                WNDCLASS    wc;                        // Windows Class Structure
                DWORD        dwExStyle;                // Window Extended Style
                DWORD        dwStyle;                // Window Style
                RECT        WindowRect;                // Grabs Rectangle Upper Left / Lower Right Values
                WindowRect.left = (long) 0;            // Set Left Value To 0
                WindowRect.right = (long) parameters.width;        // Set Right Value To Requested Width
                WindowRect.top = (long) 0;                // Set Top Value To 0
                WindowRect.bottom = (long) parameters.height;        // Set Bottom Value To Requested Height

                m_hInstance = GetModuleHandleW(NULL);                // Grab An Instance For Our Window
                wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;    // Redraw On Size, And Own DC For Window.
                wc.lpfnWndProc = (WNDPROC) WndProc;                    // WndProc Handles Messages
                wc.cbClsExtra = 0;                                    // No Extra Window Data
                wc.cbWndExtra = 0;                                    // No Extra Window Data
                wc.hInstance = m_hInstance;                            // Set The Instance
                wc.hIcon = LoadIconW(NULL, IDI_WINLOGO);            // Load The Default Icon
                wc.hCursor = LoadCursorW(NULL, IDC_ARROW);            // Load The Arrow Pointer
                wc.hbrBackground = NULL;                                    // No Background Required For GL
                wc.lpszMenuName = NULL;                                    // We Don't Want A Menu
                wc.lpszClassName = L"OpenGL";                                // Set The Class Name

                if (!RegisterClassW(&wc))                                    // Attempt To Register The Window Class
                {
                    MessageBoxW(NULL, L"Failed To Register The Window Class.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
                    throw WindowCreateException("Failed To Register The Window Class.");
                }

                if (m_Fullscreen)                                                // Are We Still In Fullscreen Mode?
                {
                    enableFullScreen(parameters.width, parameters.height, parameters.bits);
                    dwExStyle = WS_EX_APPWINDOW;                                // Window Extended Style
                    dwStyle = WS_POPUP;                                        // Windows Style
                    ShowCursor(FALSE);                                        // Hide Mouse Pointer
                }
                else
                {
                    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;            // Window Extended Style
                    dwStyle = WS_OVERLAPPEDWINDOW;                            // Windows Style
                }

                AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);        // Adjust Window To True Requested Size


                // This thing is necessary to cooperate with multibyte API.
                std::wstring wideTitle(parameters.title.begin(), parameters.title.end());

                // Create The Window
                if (!(m_hWnd = CreateWindowExW(dwExStyle,                            // Extended Style For The Window
                    L"OpenGL",                            // Class Name
                    wideTitle.c_str(),                           // Window Title
                    dwStyle |                                // Defined Window Style
                    WS_CLIPSIBLINGS |                        // Required Window Style
                    WS_CLIPCHILDREN,                         // Required Window Style
                    0, 0,                                    // Window Position
                    WindowRect.right - WindowRect.left,      // Calculate Window Width
                    WindowRect.bottom - WindowRect.top,      // Calculate Window Height
                    NULL,                                    // No Parent Window
                    NULL,                                    // No Menu
                    m_hInstance,                             // Instance
                    this)))
                {
                    throw WindowCreateException("Window Creation Error");
                }

                static PIXELFORMATDESCRIPTOR pfd =                // pfd Tells Windows How We Want Things To Be
                {
                    sizeof(PIXELFORMATDESCRIPTOR),                // Size Of This Pixel Format Descriptor
                    1,                                            // Version Number
                    PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
                    PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
                    PFD_DOUBLEBUFFER,                            // Must Support Double Buffering
                    PFD_TYPE_RGBA,                                // Request An RGBA Format
                    parameters.bits,                             // Select Our Color Depth
                    0, 0, 0, 0, 0, 0,                            // Color Bits Ignored
                    0,                                            // No Alpha Buffer
                    0,                                            // Shift Bit Ignored
                    0,                                            // No Accumulation Buffer
                    0, 0, 0, 0,                                    // Accumulation Bits Ignored
                    16,                                            // 16Bit Z-Buffer (Depth Buffer)  
                    8,                                            // No Stencil Buffer
                    0,                                            // No Auxiliary Buffer
                    PFD_MAIN_PLANE,                                // Main Drawing Layer
                    0,                                            // Reserved
                    0, 0, 0                                        // Layer Masks Ignored
                };
                if (!(m_hDC = GetDC(m_hWnd))) {
                    // Did We Get A Device Context?
                    throw WindowCreateException("Can't Create A GL Device Context.");
                }
                if (!(PixelFormat = ChoosePixelFormat(m_hDC, &pfd))) {
                    // Did Windows Find A Matching Pixel Format?
                    throw WindowCreateException("Can't Find A Suitable PixelFormat.");
                }
                if (!SetPixelFormat(m_hDC, PixelFormat, &pfd)) {
                    // Are We Able To Set The Pixel Format?
                    throw WindowCreateException("Can't Set The PixelFormat.");
                }

                HGLRC BaselineContext;
                if (!(BaselineContext = wglCreateContext(m_hDC))) {
                    // Are We Able To Get A Rendering Context?
                    throw WindowCreateException("Can't Create A GL Rendering Context.");
                }
                if (!wglMakeCurrent(m_hDC, BaselineContext)) {
                    // Try To Activate The Rendering Context{
                    throw WindowCreateException("Can't Activate The GL Rendering Context.");
                }

                if (contextCreator) {
                    HGLRC newContext = contextCreator(m_hDC);
                    // try setting up the new context
                    if (wglMakeCurrent(m_hDC, newContext)) {
                        m_hRC = newContext;
                        wglDeleteContext(BaselineContext);
                    }
                    else {
                        // fallback to old context
                        m_hRC = BaselineContext;
                        wglMakeCurrent(m_hDC, BaselineContext);
                    }
                } 
                else {
                    // no not-baseline setup provided
                    m_hRC = BaselineContext;
                }

                ShowWindow(m_hWnd, SW_SHOW);                        // Show The Window
                SetForegroundWindow(m_hWnd);                        // Slightly Higher Priority
                SetFocus(m_hWnd);                                    // Sets Keyboard Focus To The Window
            }
            catch (WindowCreateException const&) {
                kill();
            }
        }

       


        ~WinAPIOGLWindow() {
            try {
                kill();
            }
            catch (WindowException&) {
                // TODO: error reporting here
            }
            catch (...) {
            }
        }
    };

    typedef WinAPIOGLWindow Window;

    

#else
#error "This OS is not supported."
#endif


}

