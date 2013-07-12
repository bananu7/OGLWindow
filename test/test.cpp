#include <iostream>
#include <stdexcept>

//#include <glload/gl_3_3.hpp>
//#include <glload/gl_load.hpp>
//#include "GLId.h"

//#include <boost/exception.hpp>

//#include "Texture.hpp"
//  #include "VertexAttributeArray.h"
//#include "VertexBuffer.h"
#include <vector>

#include "OpenGLWindow.hpp"

int main() {
    try {
        oglw::Window win;

        /*glload::LoadFunctions();

        if (!gl::exts::var_ARB_debug_output)
            throw ContextException() << str_info("ARB_debug_output not available");
        if (!gl::exts::var_EXT_direct_state_access)
            throw ContextException() << str_info("EXT_direct_state_access output not available");*/

        //BOOST_THROW_EXCEPTION(ContextException() << str_info("test"));

        win.setKeydownCb([&win](int key){ printf("lol"); });

        //gldr::VertexAttributeArray vao;
        //gldr::Texture2d tex;

        while ( win.display(), win.process() );
    }
    catch (const std::exception& e) {
        std::cerr << e.what();
    }
}