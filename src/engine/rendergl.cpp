// rendergl.cpp: core opengl rendering stuff

#include "pch.h"
#include "engine.h"

bool hasVBO = false, hasDRE = false, hasOQ = false, hasTR = false, hasFBO = false, hasDS = false, hasTF = false, hasBE = false, hasBC = false, hasCM = false, hasNP2 = false, hasTC = false, hasTE = false, hasMT = false, hasD3 = false, hasstencil = false, hasAF = false, hasVP2 = false, hasVP3 = false, hasPP = false, hasMDA = false, hasTE3 = false, hasTE4 = false, hasVP = false, hasFP = false, hasGLSL = false, hasGM = false, hasNVFB = false;

VAR(renderpath, 1, 0, 0);

// GL_ARB_vertex_buffer_object
PFNGLGENBUFFERSARBPROC       glGenBuffers_       = NULL;
PFNGLBINDBUFFERARBPROC       glBindBuffer_       = NULL;
PFNGLMAPBUFFERARBPROC        glMapBuffer_        = NULL;
PFNGLUNMAPBUFFERARBPROC      glUnmapBuffer_      = NULL;
PFNGLBUFFERDATAARBPROC       glBufferData_       = NULL;
PFNGLBUFFERSUBDATAARBPROC    glBufferSubData_    = NULL;
PFNGLDELETEBUFFERSARBPROC    glDeleteBuffers_    = NULL;
PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubData_ = NULL;

// GL_ARB_multitexture
PFNGLACTIVETEXTUREARBPROC		glActiveTexture_		= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_ = NULL;
PFNGLMULTITEXCOORD2FARBPROC	 glMultiTexCoord2f_	 = NULL;
PFNGLMULTITEXCOORD3FARBPROC	 glMultiTexCoord3f_	 = NULL;
PFNGLMULTITEXCOORD4FARBPROC  glMultiTexCoord4f_     = NULL;

// GL_ARB_vertex_program, GL_ARB_fragment_program
PFNGLGENPROGRAMSARBPROC			glGenPrograms_			= NULL;
PFNGLDELETEPROGRAMSARBPROC		 glDeletePrograms_		 = NULL;
PFNGLBINDPROGRAMARBPROC			glBindProgram_			= NULL;
PFNGLPROGRAMSTRINGARBPROC		  glProgramString_		  = NULL;
PFNGLGETPROGRAMIVARBPROC           glGetProgramiv_           = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC  glProgramEnvParameter4f_  = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fv_ = NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC  glEnableVertexAttribArray_  = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArray_ = NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC      glVertexAttribPointer_      = NULL;

// GL_EXT_gpu_program_parameters
PFNGLPROGRAMENVPARAMETERS4FVEXTPROC   glProgramEnvParameters4fv_   = NULL;
PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC glProgramLocalParameters4fv_ = NULL;

// GL_ARB_occlusion_query
PFNGLGENQUERIESARBPROC		glGenQueries_		= NULL;
PFNGLDELETEQUERIESARBPROC	 glDeleteQueries_	 = NULL;
PFNGLBEGINQUERYARBPROC		glBeginQuery_		= NULL;
PFNGLENDQUERYARBPROC		  glEndQuery_		  = NULL;
PFNGLGETQUERYIVARBPROC		glGetQueryiv_		= NULL;
PFNGLGETQUERYOBJECTIVARBPROC  glGetQueryObjectiv_  = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuiv_ = NULL;

// GL_EXT_framebuffer_object
PFNGLBINDRENDERBUFFEREXTPROC		glBindRenderbuffer_		= NULL;
PFNGLDELETERENDERBUFFERSEXTPROC	 glDeleteRenderbuffers_	 = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC		 glGenRenderbuffers_		= NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC	 glRenderbufferStorage_	 = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus_  = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC		 glBindFramebuffer_		 = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC	  glDeleteFramebuffers_	  = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC		 glGenFramebuffers_		 = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC	glFramebufferTexture2D_	= NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer_ = NULL;
PFNGLGENERATEMIPMAPEXTPROC		  glGenerateMipmap_		  = NULL;

// GL_ARB_shading_language_100, GL_ARB_shader_objects, GL_ARB_fragment_shader, GL_ARB_vertex_shader
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObject_	  = NULL;
PFNGLDELETEOBJECTARBPROC			  glDeleteObject_			 = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		  glUseProgramObject_		 = NULL;
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObject_		= NULL;
PFNGLSHADERSOURCEARBPROC			  glShaderSource_			 = NULL;
PFNGLCOMPILESHADERARBPROC			 glCompileShader_			= NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	  glGetObjectParameteriv_	 = NULL;
PFNGLATTACHOBJECTARBPROC			  glAttachObject_			 = NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLog_				= NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgram_			  = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocation_		= NULL;
PFNGLUNIFORM4FVARBPROC				glUniform4fv_				= NULL;
PFNGLUNIFORM1IARBPROC				 glUniform1i_				= NULL;

// GL_EXT_draw_range_elements
PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements_ = NULL;

// GL_EXT_blend_minmax
PFNGLBLENDEQUATIONEXTPROC glBlendEquation_ = NULL;

// GL_EXT_blend_color
PFNGLBLENDCOLOREXTPROC glBlendColor_ = NULL;

// GL_EXT_multi_draw_arrays
PFNGLMULTIDRAWARRAYSEXTPROC   glMultiDrawArrays_ = NULL;
PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElements_ = NULL;

void *getprocaddress(const char *name)
{
	return SDL_GL_GetProcAddress(name);
}

VARP(ati_skybox_bug, 0, 0, 1);
VAR(ati_texgen_bug, 0, 0, 1);
VAR(ati_oq_bug, 0, 0, 1);
VAR(ati_minmax_bug, 0, 0, 1);
VAR(ati_dph_bug, 0, 0, 1);
VAR(nvidia_texgen_bug, 0, 0, 1);
VAR(nvidia_scissor_bug, 0, 0, 1);
VAR(apple_glsldepth_bug, 0, 0, 1);
VAR(apple_ff_bug, 0, 0, 1);
VAR(apple_vp_bug, 0, 0, 1);
VAR(intel_quadric_bug, 0, 0, 1);
VAR(mesa_program_bug, 0, 0, 1);
VAR(avoidshaders, 1, 0, 0);
VAR(minimizetcusage, 1, 0, 0);
VAR(emulatefog, 1, 0, 0);
VAR(usevp2, 1, 0, 0);
VAR(usevp3, 1, 0, 0);
VAR(usetexrect, 1, 0, 0);
VAR(rtscissor, 0, 1, 1);
VAR(blurtile, 0, 1, 1);
VAR(rtsharefb, 0, 1, 1);

static bool checkseries(const char *s, int low, int high)
{
    while(*s && !isdigit(*s)) ++s;
    if(!*s) return false;
    int n = 0;
    while(isdigit(*s)) n = n*10 + (*s++ - '0');
    return n >= low && n < high;
}

void gl_checkextensions()
{
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    conoutf("\fmrenderer: %s (%s)", renderer, vendor);
    conoutf("\fmdriver: %s", version);

#ifdef __APPLE__
    extern int mac_osversion();
    int osversion = mac_osversion();  /* 0x1050 = 10.5 (Leopard) */
#endif

    //extern int shaderprecision;
    // default to low precision shaders on certain cards, can be overridden with -f3
    // char *weakcards[] = { "GeForce FX", "Quadro FX", "6200", "9500", "9550", "9600", "9700", "9800", "X300", "X600", "FireGL", "Intel", "Chrome", NULL }
    // if(shaderprecision==2) for(char **wc = weakcards; *wc; wc++) if(strstr(renderer, *wc)) shaderprecision = 1;

    if(strstr(exts, "GL_EXT_texture_env_combine") || strstr(exts, "GL_ARB_texture_env_combine"))
    {
        hasTE = true;
        if(strstr(exts, "GL_ATI_texture_env_combine3")) hasTE3 = true;
        if(strstr(exts, "GL_NV_texture_env_combine4")) hasTE4 = true;
        if(strstr(exts, "GL_EXT_texture_env_dot3") || strstr(exts, "GL_ARB_texture_env_dot3")) hasD3 = true;
    }
    else conoutf("\frWARNING: No texture_env_combine extension! (your video card is WAY too old)");

    if(strstr(exts, "GL_ARB_multitexture"))
    {
        glActiveTexture_       = (PFNGLACTIVETEXTUREARBPROC)      getprocaddress("glActiveTextureARB");
        glClientActiveTexture_ = (PFNGLCLIENTACTIVETEXTUREARBPROC)getprocaddress("glClientActiveTextureARB");
        glMultiTexCoord2f_     = (PFNGLMULTITEXCOORD2FARBPROC)    getprocaddress("glMultiTexCoord2fARB");
        glMultiTexCoord3f_     = (PFNGLMULTITEXCOORD3FARBPROC)    getprocaddress("glMultiTexCoord3fARB");
        glMultiTexCoord4f_     = (PFNGLMULTITEXCOORD4FARBPROC)    getprocaddress("glMultiTexCoord4fARB");
        hasMT = true;
    }
    else conoutf("\frWARNING: No multitexture extension!");

    if(strstr(exts, "GL_ARB_vertex_buffer_object"))
    {
        glGenBuffers_       = (PFNGLGENBUFFERSARBPROC)      getprocaddress("glGenBuffersARB");
        glBindBuffer_       = (PFNGLBINDBUFFERARBPROC)      getprocaddress("glBindBufferARB");
        glMapBuffer_        = (PFNGLMAPBUFFERARBPROC)       getprocaddress("glMapBufferARB");
        glUnmapBuffer_      = (PFNGLUNMAPBUFFERARBPROC)     getprocaddress("glUnmapBufferARB");
        glBufferData_       = (PFNGLBUFFERDATAARBPROC)      getprocaddress("glBufferDataARB");
        glBufferSubData_    = (PFNGLBUFFERSUBDATAARBPROC)   getprocaddress("glBufferSubDataARB");
        glDeleteBuffers_    = (PFNGLDELETEBUFFERSARBPROC)   getprocaddress("glDeleteBuffersARB");
        glGetBufferSubData_ = (PFNGLGETBUFFERSUBDATAARBPROC)getprocaddress("glGetBufferSubDataARB");
        hasVBO = true;
        //conoutf("\frUsing GL_ARB_vertex_buffer_object extension.");
    }
    else conoutf("\frWARNING: No vertex_buffer_object extension! (geometry heavy maps will be SLOW)");

    if(strstr(exts, "GL_EXT_draw_range_elements"))
    {
        glDrawRangeElements_ = (PFNGLDRAWRANGEELEMENTSEXTPROC)getprocaddress("glDrawRangeElementsEXT");
        hasDRE = true;
    }

    if(strstr(exts, "GL_EXT_multi_draw_arrays"))
    {
        glMultiDrawArrays_   = (PFNGLMULTIDRAWARRAYSEXTPROC)  getprocaddress("glMultiDrawArraysEXT");
        glMultiDrawElements_ = (PFNGLMULTIDRAWELEMENTSEXTPROC)getprocaddress("glMultiDrawElementsEXT");
        hasMDA = true;
    }

#ifdef __APPLE__
    // floating point FBOs not fully supported until 10.5
    if(osversion>=0x1050)
#endif
    if(strstr(exts, "GL_ARB_texture_float") || strstr(exts, "GL_ATI_texture_float"))
    {
        hasTF = true;
        //conoutf("\frUsing GL_ARB_texture_float extension");
        shadowmap = 1;
        extern int smoothshadowmappeel;
        smoothshadowmappeel = 1;
    }

    if(strstr(exts, "GL_NV_float_buffer")) hasNVFB = true;

    if(strstr(exts, "GL_EXT_framebuffer_object"))
    {
        glBindRenderbuffer_        = (PFNGLBINDRENDERBUFFEREXTPROC)       getprocaddress("glBindRenderbufferEXT");
        glDeleteRenderbuffers_     = (PFNGLDELETERENDERBUFFERSEXTPROC)    getprocaddress("glDeleteRenderbuffersEXT");
        glGenRenderbuffers_        = (PFNGLGENFRAMEBUFFERSEXTPROC)        getprocaddress("glGenRenderbuffersEXT");
        glRenderbufferStorage_     = (PFNGLRENDERBUFFERSTORAGEEXTPROC)    getprocaddress("glRenderbufferStorageEXT");
        glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) getprocaddress("glCheckFramebufferStatusEXT");
        glBindFramebuffer_         = (PFNGLBINDFRAMEBUFFEREXTPROC)        getprocaddress("glBindFramebufferEXT");
        glDeleteFramebuffers_      = (PFNGLDELETEFRAMEBUFFERSEXTPROC)     getprocaddress("glDeleteFramebuffersEXT");
        glGenFramebuffers_         = (PFNGLGENFRAMEBUFFERSEXTPROC)        getprocaddress("glGenFramebuffersEXT");
        glFramebufferTexture2D_    = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)   getprocaddress("glFramebufferTexture2DEXT");
        glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)getprocaddress("glFramebufferRenderbufferEXT");
        glGenerateMipmap_          = (PFNGLGENERATEMIPMAPEXTPROC)         getprocaddress("glGenerateMipmapEXT");
        hasFBO = true;
        //conoutf("\frUsing GL_EXT_framebuffer_object extension.");
    }
    else conoutf("\frWARNING: No framebuffer object support. (reflective water may be slow)");

    if(strstr(exts, "GL_ARB_occlusion_query"))
    {
        GLint bits;
        glGetQueryiv_ = (PFNGLGETQUERYIVARBPROC)getprocaddress("glGetQueryivARB");
        glGetQueryiv_(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &bits);
        if(bits)
        {
            glGenQueries_ =        (PFNGLGENQUERIESARBPROC)       getprocaddress("glGenQueriesARB");
            glDeleteQueries_ =     (PFNGLDELETEQUERIESARBPROC)    getprocaddress("glDeleteQueriesARB");
            glBeginQuery_ =        (PFNGLBEGINQUERYARBPROC)       getprocaddress("glBeginQueryARB");
            glEndQuery_ =          (PFNGLENDQUERYARBPROC)         getprocaddress("glEndQueryARB");
            glGetQueryObjectiv_ =  (PFNGLGETQUERYOBJECTIVARBPROC) getprocaddress("glGetQueryObjectivARB");
            glGetQueryObjectuiv_ = (PFNGLGETQUERYOBJECTUIVARBPROC)getprocaddress("glGetQueryObjectuivARB");
            hasOQ = true;
            //conoutf("\frUsing GL_ARB_occlusion_query extension.");
#if defined(__APPLE__) && SDL_BYTEORDER == SDL_BIG_ENDIAN
            if(strstr(vendor, "ATI") && (osversion<0x1050)) ati_oq_bug = 1;
#endif
            if(ati_oq_bug) conoutf("\frWARNING: Using ATI occlusion query bug workaround. (use \"/ati_oq_bug 0\" to disable if unnecessary)");
        }
    }
    if(!hasOQ)
    {
        conoutf("\frWARNING: No occlusion query support! (large maps may be SLOW)");
        zpass = 0;
        extern int vacubesize;
        vacubesize = 64;
        waterreflect = 0;
    }

    extern int reservedynlighttc, reserveshadowmaptc, maxtexsize, batchlightmaps;
    if(strstr(vendor, "ATI"))
    {
        floatvtx = 1;
        //conoutf("\frWARNING: ATI cards may show garbage in skybox. (use \"/ati_skybox_bug 1\" to fix)");

        reservedynlighttc = 2;
        reserveshadowmaptc = 3;
        minimizetcusage = 1;
        emulatefog = 1;
        extern int depthfxprecision;
        if(hasTF) depthfxprecision = 1;

        //ati_texgen_bug = 1;
    }
    else if(strstr(vendor, "NVIDIA"))
    {
        reservevpparams = 10;
        rtsharefb = 0; // work-around for strange driver stalls involving when using many FBOs
        extern int filltjoints;
        if(!strstr(exts, "GL_EXT_gpu_shader4")) filltjoints = 0; // DX9 or less NV cards seem to not cause many sparklies

        nvidia_texgen_bug = 1;
        if(hasFBO && !hasTF) nvidia_scissor_bug = 1; // 5200 bug, clearing with scissor on an FBO messes up on reflections, may affect lesser cards too
        extern int fpdepthfx;
        if(hasTF && (!strstr(renderer, "GeForce") || !checkseries(renderer, 6000, 6600)))
            fpdepthfx = 1; // FP filtering causes software fallback on 6200?
    }
    else if(strstr(vendor, "Intel"))
    {
        avoidshaders = 1;
        intel_quadric_bug = 1;
        maxtexsize = 256;
        reservevpparams = 20;
        batchlightmaps = 0;

        if(!hasOQ) waterrefract = 0;

#ifdef __APPLE__
        apple_vp_bug = 1;
#endif
    }
    else if(strstr(vendor, "Tungsten") || strstr(vendor, "Mesa") || strstr(vendor, "Microsoft") || strstr(vendor, "S3 Graphics"))
    {
        avoidshaders = 1;
        floatvtx = 1;
        maxtexsize = 256;
        reservevpparams = 20;
        batchlightmaps = 0;

        if(!hasOQ) waterrefract = 0;
    }
    //if(floatvtx) conoutf("\frWARNING: Using floating point vertexes. (use \"/floatvtx 0\" to disable)");

    if(strstr(exts, "GL_ARB_vertex_program") && strstr(exts, "GL_ARB_fragment_program"))
    {
        hasVP = hasFP = true;
        glGenPrograms_ =              (PFNGLGENPROGRAMSARBPROC)              getprocaddress("glGenProgramsARB");
        glDeletePrograms_ =           (PFNGLDELETEPROGRAMSARBPROC)           getprocaddress("glDeleteProgramsARB");
        glBindProgram_ =              (PFNGLBINDPROGRAMARBPROC)              getprocaddress("glBindProgramARB");
        glProgramString_ =            (PFNGLPROGRAMSTRINGARBPROC)            getprocaddress("glProgramStringARB");
        glGetProgramiv_ =             (PFNGLGETPROGRAMIVARBPROC)             getprocaddress("glGetProgramivARB");
        glProgramEnvParameter4f_ =    (PFNGLPROGRAMENVPARAMETER4FARBPROC)    getprocaddress("glProgramEnvParameter4fARB");
        glProgramEnvParameter4fv_ =   (PFNGLPROGRAMENVPARAMETER4FVARBPROC)   getprocaddress("glProgramEnvParameter4fvARB");
        glEnableVertexAttribArray_ =  (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)  getprocaddress("glEnableVertexAttribArrayARB");
        glDisableVertexAttribArray_ = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) getprocaddress("glDisableVertexAttribArrayARB");
        glVertexAttribPointer_ =      (PFNGLVERTEXATTRIBPOINTERARBPROC)      getprocaddress("glVertexAttribPointerARB");

        if(strstr(exts, "GL_ARB_shading_language_100") && strstr(exts, "GL_ARB_shader_objects") && strstr(exts, "GL_ARB_vertex_shader") && strstr(exts, "GL_ARB_fragment_shader"))
        {
            glCreateProgramObject_ =        (PFNGLCREATEPROGRAMOBJECTARBPROC)     getprocaddress("glCreateProgramObjectARB");
            glDeleteObject_ =               (PFNGLDELETEOBJECTARBPROC)            getprocaddress("glDeleteObjectARB");
            glUseProgramObject_ =           (PFNGLUSEPROGRAMOBJECTARBPROC)        getprocaddress("glUseProgramObjectARB");
            glCreateShaderObject_ =         (PFNGLCREATESHADEROBJECTARBPROC)      getprocaddress("glCreateShaderObjectARB");
            glShaderSource_ =               (PFNGLSHADERSOURCEARBPROC)            getprocaddress("glShaderSourceARB");
            glCompileShader_ =              (PFNGLCOMPILESHADERARBPROC)           getprocaddress("glCompileShaderARB");
            glGetObjectParameteriv_ =       (PFNGLGETOBJECTPARAMETERIVARBPROC)    getprocaddress("glGetObjectParameterivARB");
            glAttachObject_ =               (PFNGLATTACHOBJECTARBPROC)            getprocaddress("glAttachObjectARB");
            glGetInfoLog_ =                 (PFNGLGETINFOLOGARBPROC)              getprocaddress("glGetInfoLogARB");
            glLinkProgram_ =                (PFNGLLINKPROGRAMARBPROC)             getprocaddress("glLinkProgramARB");
            glGetUniformLocation_ =         (PFNGLGETUNIFORMLOCATIONARBPROC)      getprocaddress("glGetUniformLocationARB");
            glUniform4fv_ =                 (PFNGLUNIFORM4FVARBPROC)              getprocaddress("glUniform4fvARB");
            glUniform1i_ =                  (PFNGLUNIFORM1IARBPROC)               getprocaddress("glUniform1iARB");

            extern bool checkglslsupport();
            if(checkglslsupport())
            {
                hasGLSL = true;
#ifdef __APPLE__
                //if(osversion<0x1050) ??
                apple_glsldepth_bug = 1;
#endif
                if(apple_glsldepth_bug) conoutf("\frWARNING: Using Apple GLSL depth bug workaround. (use \"/apple_glsldepth_bug 0\" to disable if unnecessary");
            }
        }

        if(strstr(vendor, "ATI")) ati_dph_bug = 1;
        else if(strstr(vendor, "Tungsten")) mesa_program_bug = 1;

#ifdef __APPLE__
        if(osversion>=0x1050)
        {
            apple_ff_bug = 1;
            conoutf("\frWARNING: Using Leopard ARB_position_invariant bug workaround. (use \"/apple_ff_bug 0\" to disable if unnecessary)");
        }
#endif
    }

    if(strstr(exts, "GL_NV_vertex_program2_option")) { usevp2 = 1; hasVP2 = true; }
    if(strstr(exts, "GL_NV_vertex_program3")) { usevp3 = 1; hasVP3 = true; }

    if(strstr(exts, "GL_EXT_gpu_program_parameters"))
    {
        glProgramEnvParameters4fv_   = (PFNGLPROGRAMENVPARAMETERS4FVEXTPROC)  getprocaddress("glProgramEnvParameters4fvEXT");
        glProgramLocalParameters4fv_ = (PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC)getprocaddress("glProgramLocalParameters4fvEXT");
        hasPP = true;
    }

    if(strstr(exts, "GL_EXT_texture_rectangle") || strstr(exts, "GL_ARB_texture_rectangle"))
    {
        usetexrect = 1;
        hasTR = true;
        //conoutf("\frUsing GL_ARB_texture_rectangle extension.");
    }
    else if(hasMT && hasVP && hasFP) conoutf("\frWARNING: No texture rectangle support. (no full screen shaders)");

    if(strstr(exts, "GL_EXT_packed_depth_stencil") || strstr(exts, "GL_NV_packed_depth_stencil"))
    {
        hasDS = true;
        //conoutf("\frUsing GL_EXT_packed_depth_stencil extension.");
    }

    if(strstr(exts, "GL_EXT_blend_minmax"))
    {
        glBlendEquation_ = (PFNGLBLENDEQUATIONEXTPROC) getprocaddress("glBlendEquationEXT");
        hasBE = true;
        if(strstr(vendor, "ATI")) ati_minmax_bug = 1;
        //conoutf("\frUsing GL_EXT_blend_minmax extension.");
    }

    if(strstr(exts, "GL_EXT_blend_color"))
    {
        glBlendColor_ = (PFNGLBLENDCOLOREXTPROC) getprocaddress("glBlendColorEXT");
        hasBC = true;
        //conoutf("\frUsing GL_EXT_blend_color extension.");
    }

    if(strstr(exts, "GL_ARB_texture_cube_map"))
    {
        GLint val;
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &val);
        hwcubetexsize = val;
        hasCM = true;
        //conoutf("\frUsing GL_ARB_texture_cube_map extension.");
    }
    else conoutf("\frWARNING: No cube map texture support. (no reflective glass)");

    if(strstr(exts, "GL_ARB_texture_non_power_of_two"))
    {
        hasNP2 = true;
        //conoutf("\frUsing GL_ARB_texture_non_power_of_two extension.");
    }
    else conoutf("\frWARNING: Non-power-of-two textures not supported!");

    if(strstr(exts, "GL_EXT_texture_compression_s3tc"))
    {
        hasTC = true;
        //conoutf("\frUsing GL_EXT_texture_compression_s3tc extension.");
    }

    if(strstr(exts, "GL_EXT_texture_filter_anisotropic"))
    {
       GLint val;
       glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
       hwmaxaniso = val;
       hasAF = true;
       //conoutf("\frUsing GL_EXT_texture_filter_anisotropic extension.");
    }

    if(strstr(exts, "GL_SGIS_generate_mipmap"))
    {
        hasGM = true;
        //conoutf("\frUsing GL_SGIS_generate_mipmap extension.");
    }

    GLint val;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
    hwtexsize = val;
}

void glext(char *ext)
{
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    intret(strstr(exts, ext) ? 1 : 0);
}
COMMAND(glext, "s");

void gl_init(int w, int h, int bpp, int depth, int fsaa)
{
	#define fogvalues 0.5f, 0.6f, 0.7f, 1.0f

	glViewport(0, 0, w, h);
	glClearColor(fogvalues);
	glClearDepth(1);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);


	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.25f);
	glHint(GL_FOG_HINT, GL_NICEST);
	GLfloat fogcolor[4] = { fogvalues };
	glFogfv(GL_FOG_COLOR, fogcolor);


	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

    extern int useshaders;
    if(!useshaders || (useshaders<0 && avoidshaders) || !hasMT || !hasVP || !hasFP)
    {
        if(!hasMT || !hasVP || !hasFP) conoutf("\frWARNING: No shader support! Using fixed-function fallback. (no fancy visuals for you)");
        else if(useshaders<0 && !hasTF) conoutf("\frWARNING: Disabling shaders for extra performance. (use \"/shaders 1\" to enable shaders if desired)");
        renderpath = R_FIXEDFUNCTION;
        conoutf("\fmRendering using the OpenGL 1.5 fixed-function path.");
        if(ati_texgen_bug) conoutf("\frWARNING: Using ATI texgen bug workaround. (use \"/ati_texgen_bug 0\" to disable if unnecessary)");
        if(nvidia_texgen_bug) conoutf("\frWARNING: Using NVIDIA texgen bug workaround. (use \"/nvidia_texgen_bug 0\" to disable if unnecessary)");
    }
    else
    {
        renderpath = hasGLSL ? R_GLSLANG : R_ASMSHADER;
        if(renderpath==R_GLSLANG) conoutf("\fmRendering using the OpenGL 1.5 GLSL shader path.");
        else conoutf("\fmRendering using the OpenGL 1.5 assembly shader path.");
    }

    if(fsaa) glEnable(GL_MULTISAMPLE);

    inittmus();
    setuptexcompress();
}

void cleanupgl()
{
    if(glIsEnabled(GL_MULTISAMPLE)) glDisable(GL_MULTISAMPLE);

    extern int nomasks, nolights, nowater;
    nomasks = nolights = nowater = 0;
}

VAR(wireframe, 0, 0, 1);

physent camera, *camera1 = &camera;
vec worldpos, camerapos, camdir, camright, camup;

void findorientation(vec &o, float yaw, float pitch, vec &pos)
{
	vec dir;
	vecfromyawpitch(yaw, pitch, 1, 0, dir);
	if(raycubepos(o, dir, pos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
		pos = dir.mul(2*hdr.worldsize).add(o); //otherwise 3dgui won't work when outside of map
}

void transplayer()
{
	glLoadIdentity();

	glRotatef(camera1->roll, 0, 0, 1);
	glRotatef(camera1->pitch, -1, 0, 0);
	glRotatef(camera1->yaw, 0, 1, 0);

	// move from RH to Z-up LH quake style worldspace
	glRotatef(-90, 1, 0, 0);
	glScalef(1, -1, 1);

	glTranslatef(-camera1->o.x, -camera1->o.y, -camera1->o.z);
}

float curfov = 100, fovy, aspect;
int farplane;
int xtraverts, xtravertsva;

VARW(fog, 16, 4000, INT_MAX-1);
VARW(fogcolour, 0, 0x8099B3, 0xFFFFFF);

void vecfromcursor(float x, float y, float z, vec &dir)
{
	GLdouble cmvm[16], cpjm[16];
	GLint view[4];
	GLdouble dx, dy, dz;

	glGetDoublev(GL_MODELVIEW_MATRIX, cmvm);
	glGetDoublev(GL_PROJECTION_MATRIX, cpjm);
	glGetIntegerv(GL_VIEWPORT, view);

	gluUnProject(x*float(view[2]), (1.f-y)*float(view[3]), z, cmvm, cpjm, view, &dx, &dy, &dz);
	dir = vec((float)dx, (float)dy, (float)dz);
	gluUnProject(x*float(view[2]), (1.f-y)*float(view[3]), 0.0f, cmvm, cpjm, view, &dx, &dy, &dz);
	dir.sub(vec((float)dx, (float)dy, (float)dz));
	dir.normalize();
}

void vectocursor(vec &v, float &x, float &y, float &z)
{
	GLdouble cmvm[16], cpjm[16];
	GLint view[4];
	GLdouble dx, dy, dz;

	glGetDoublev(GL_MODELVIEW_MATRIX, cmvm);
	glGetDoublev(GL_PROJECTION_MATRIX, cpjm);
	glGetIntegerv(GL_VIEWPORT, view);

	gluProject(v.x, v.y, v.z, cmvm, cpjm, view, &dx, &dy, &dz);
	x = (float)dx;
	y = (float)view[3]-dy;
	z = (float)dz;
}

void project(float fovy, float aspect, int farplane, bool flipx, bool flipy, bool swapxy, float zscale)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(swapxy) glRotatef(90, 0, 0, 1);
    if(flipx || flipy!=swapxy || zscale!=1) glScalef(flipx ? -1 : 1, flipy!=swapxy ? -1 : 1, zscale);
    gluPerspective(fovy, aspect, 0.54f, farplane);
    glMatrixMode(GL_MODELVIEW);
}

VAR(reflectclip, 0, 6, 64);

GLfloat clipmatrix[16];

void genclipmatrix(float a, float b, float c, float d, GLfloat matrix[16])
{
	// transform the clip plane into camera space
    float clip[4];
    loopi(4) clip[i] = a*invmvmatrix[i*4 + 0] + b*invmvmatrix[i*4 + 1] + c*invmvmatrix[i*4 + 2] + d*invmvmatrix[i*4 + 3];

    memcpy(matrix, projmatrix, 16*sizeof(GLfloat));

	float x = ((clip[0]<0 ? -1 : (clip[0]>0 ? 1 : 0)) + matrix[8]) / matrix[0],
		  y = ((clip[1]<0 ? -1 : (clip[1]>0 ? 1 : 0)) + matrix[9]) / matrix[5],
		  w = (1 + matrix[10]) / matrix[14],
		  scale = 2 / (x*clip[0] + y*clip[1] - clip[2] + w*clip[3]);
	matrix[2] = clip[0]*scale;
	matrix[6] = clip[1]*scale;
	matrix[10] = clip[2]*scale + 1.0f;
	matrix[14] = clip[3]*scale;
}

void setclipmatrix(GLfloat matrix[16])
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(matrix);
	glMatrixMode(GL_MODELVIEW);
}

void undoclipmatrix()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

FVAR(polygonoffsetfactor, -3.0f);
FVAR(polygonoffsetunits, -3.0f);
FVAR(depthoffset, 0.01f);

void enablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glPolygonOffset(polygonoffsetfactor, polygonoffsetunits);
        glEnable(type);
        return;
    }

    bool clipped = reflectz < 1e15f && reflectclip;

    GLfloat offsetmatrix[16];
    memcpy(offsetmatrix, clipped ? clipmatrix : projmatrix, 16*sizeof(GLfloat));
    offsetmatrix[14] += depthoffset * projmatrix[10];

    glMatrixMode(GL_PROJECTION);
    if(!clipped) glPushMatrix();
    glLoadMatrixf(offsetmatrix);
    glMatrixMode(GL_MODELVIEW);
}

void disablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glDisable(type);
        return;
    }

    bool clipped = reflectz < 1e15f && reflectclip;

    glMatrixMode(GL_PROJECTION);
    if(clipped) glLoadMatrixf(clipmatrix);
    else glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void setfogplane(const plane &p, bool flush)
{
	static float fogselect[4] = {0, 0, 0, 0};
    if(flush)
    {
        flushenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        flushenvparamfv("fogplane", SHPARAM_VERTEX, 9, p.v);
    }
    else
    {
        setenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        setenvparamfv("fogplane", SHPARAM_VERTEX, 9, p.v);
    }
}

void setfogplane(float scale, float z, bool flush, float fadescale, float fadeoffset)
{
    float fogselect[4] = {1, fadescale, fadeoffset, 0}, fogplane[4] = {0, 0, 0, 0};
    if(scale || z)
    {
        fogselect[0] = 0;
        fogplane[2] = scale;
        fogplane[3] = -z;
    }
    if(flush)
    {
        flushenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        flushenvparamfv("fogplane", SHPARAM_VERTEX, 9, fogplane);
    }
    else
    {
        setenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        setenvparamfv("fogplane", SHPARAM_VERTEX, 9, fogplane);
    }
}

static float findsurface(int fogmat, const vec &v, int &abovemat)
{
    ivec o(v);
    do
    {
        cube &c = lookupcube(o.x, o.y, o.z);
        if(!c.ext || (c.ext->material&MATF_VOLUME) != fogmat)
        {
            abovemat = c.ext && isliquid(c.ext->material&MATF_VOLUME) ? c.ext->material&MATF_VOLUME : MAT_AIR;
            return o.z;
        }
        o.z = lu.z + lusize;
    }
    while(o.z < hdr.worldsize);
    abovemat = MAT_AIR;
    return hdr.worldsize;
}

static void blendfog(int fogmat, float blend, float logblend, float &start, float &end, float *fogc)
{
    uchar col[3];
    switch(fogmat)
    {
        case MAT_WATER:
            getwatercolour(col);
            loopk(3) fogc[k] += blend*col[k]/255.0f;
            end += logblend*min(fog, max(waterfog*4, 32));
            break;

        case MAT_LAVA:
            getlavacolour(col);
            loopk(3) fogc[k] += blend*col[k]/255.0f;
            end += logblend*min(fog, max(lavafog*4, 32));
            break;

        default:
            fogc[0] += blend*(fogcolour>>16)/255.0f;
            fogc[1] += blend*((fogcolour>>8)&255)/255.0f;
            fogc[2] += blend*(fogcolour&255)/255.0f;
            start += logblend*(fog+64)/8;
            end += logblend*fog;
            break;
    }
}

static void setfog(int fogmat, float below = 1, int abovemat = MAT_AIR)
{
    float fogc[4] = { 0, 0, 0, 1 };
    float start = 0, end = 0;
    float logscale = 256, logblend = log(1 + (logscale - 1)*below) / log(logscale);

    blendfog(fogmat, below, logblend, start, end, fogc);
    if(below < 1) blendfog(abovemat, 1-below, 1-logblend, start, end, fogc);

    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
    glFogfv(GL_FOG_COLOR, fogc);
    glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);

    if(renderpath!=R_FIXEDFUNCTION) setfogplane();
}

bool dopostfx = false;

void invalidatepostfx()
{
    dopostfx = false;
}

static void blendfogoverlay(int fogmat, float blend, float *overlay)
{
    uchar col[3];
    float maxc;
    switch(fogmat)
    {
        case MAT_WATER:
            getwatercolour(col);
            maxc = max(col[0], max(col[1], col[2]));
            loopk(3) overlay[k] += blend*max(0.4f, col[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        case MAT_LAVA:
            getlavacolour(col);
            maxc = max(col[0], max(col[1], col[2]));
            loopk(3) overlay[k] += blend*max(0.4f, col[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        default:
            loopk(3) overlay[k] += blend;
            break;
    }
}

bool renderedgame = false, renderedavatar = false;

void rendergame()
{
    cl->render();
    if(!shadowmapping) renderedgame = true;
}

void renderavatar(bool early)
{
    cl->renderavatar(early);
}

extern void viewproject(float zscale = 1);

VARP(skyboxglare, 0, 1, 1);

void drawglare()
{
    glaring = true;
    refracting = -1;

    float oldfogstart, oldfogend, oldfogcolor[4], zerofog[4] = { 0, 0, 0, 1 };
    glGetFloatv(GL_FOG_START, &oldfogstart);
    glGetFloatv(GL_FOG_END, &oldfogend);
    glGetFloatv(GL_FOG_COLOR, oldfogcolor);

    glFogi(GL_FOG_START, (fog+64)/8);
    glFogi(GL_FOG_END, fog);
    glFogfv(GL_FOG_COLOR, zerofog);

    glClearColor(0, 0, 0, 1);
    glClear((skyboxglare ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT);

    rendergeom();

    if(skyboxglare) drawskybox(farplane, false);

    renderreflectedmapmodels();
    rendergame();

    renderwater();
    rendermaterials();
    render_particles(0);

    viewproject(0.5f);
    renderavatar(false);
    viewproject();

    glFogf(GL_FOG_START, oldfogstart);
    glFogf(GL_FOG_END, oldfogend);
    glFogfv(GL_FOG_COLOR, oldfogcolor);

    refracting = 0;
    glaring = false;
}

VARP(reflectmms, 0, 1, 1);

void drawreflection(float z, bool refract, bool clear)
{
	uchar wcol[3];
	getwatercolour(wcol);
	float fogc[4] = { wcol[0]/256.0f, wcol[1]/256.0f, wcol[2]/256.0f, 1.0f };

	if(refract && !waterfog)
	{
		glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

    reflectz = z < 0 ? 1e16f : z;
    reflecting = !refract;
    refracting = refract ? (z < 0 || camera1->o.z >= z ? -1 : 1) : 0;
    fading = renderpath!=R_FIXEDFUNCTION && waterrefract && waterfade && hasFBO && z>=0;
    fogging = refracting<0 && z>=0 && (renderpath!=R_FIXEDFUNCTION || refractfog);

    float oldfogstart, oldfogend, oldfogcolor[4];
    if(renderpath==R_FIXEDFUNCTION && fogging) glDisable(GL_FOG);
    else
    {
        glGetFloatv(GL_FOG_START, &oldfogstart);
        glGetFloatv(GL_FOG_END, &oldfogend);
        glGetFloatv(GL_FOG_COLOR, oldfogcolor);

        if(fogging)
        {
            glFogi(GL_FOG_START, 0);
            glFogi(GL_FOG_END, waterfog);
            glFogfv(GL_FOG_COLOR, fogc);
        }
        else
        {
            glFogi(GL_FOG_START, (fog+64)/8);
            glFogi(GL_FOG_END, fog);
            float fogc[4] = { (fogcolour>>16)/255.0f, ((fogcolour>>8)&255)/255.0f, (fogcolour&255)/255.0f, 1.0f };
            glFogfv(GL_FOG_COLOR, fogc);
        }
    }

	if(clear)
	{
		glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

    if(reflecting)
    {
        glPushMatrix();
        glTranslatef(0, 0, 2*z);
        glScalef(1, 1, -1);

        glCullFace(GL_BACK);
    }

    if(reflectclip && z>=0)
    {
        float zoffset = reflectclip/4.0f, zclip;
        if(refracting<0)
        {
            zclip = z+zoffset;
            if(camera1->o.z<=zclip) zclip = z;
        }
        else
        {
            zclip = z-zoffset;
            if(camera1->o.z>=zclip && camera1->o.z<=z+4.0f) zclip = z;
            if(reflecting) zclip = 2*z - zclip;
        }
        genclipmatrix(0, 0, refracting>0 ? 1 : -1, refracting>0 ? -zclip : zclip, clipmatrix);
        setclipmatrix(clipmatrix);
    }


    renderreflectedgeom(refracting<0 && z>=0 && caustics, fogging);

    if(reflecting || refracting>0 || z<0)
    {
        if(fading) glColorMask(COLORMASK, GL_TRUE);
        if(reflectclip && z>=0) undoclipmatrix();
        drawskybox(farplane, false);
        if(reflectclip && z>=0) setclipmatrix(clipmatrix);
        if(fading) glColorMask(COLORMASK, GL_FALSE);
    }
    else if(fading) glColorMask(COLORMASK, GL_FALSE);

    if(reflectmms) renderreflectedmapmodels();
    rendergame();

    if(fogging) setfogplane(1, z);
    if(refracting) rendergrass();
    renderdecals(0);
    renderportals(0);
    rendermaterials();
    render_particles(0);

    renderavatar(false);

    if(fading) glColorMask(COLORMASK, GL_TRUE);

    if(fogging) setfogplane();

    if(reflectclip && z>=0) undoclipmatrix();

    if(reflecting)
    {
        glPopMatrix();

        glCullFace(GL_FRONT);
    }

    if(renderpath==R_FIXEDFUNCTION && fogging) glEnable(GL_FOG);
    else
	{
		glFogf(GL_FOG_START, oldfogstart);
		glFogf(GL_FOG_END, oldfogend);
		glFogfv(GL_FOG_COLOR, oldfogcolor);
	}

    reflectz = 1e16f;
    refracting = 0;
    reflecting = fading = fogging = false;
}

bool envmapping = false;

void drawcubemap(int size, int level, const vec &o, float yaw, float pitch, bool flipx, bool flipy, bool swapxy)
{
	float fovx = 90.f, fovy = 90.f, aspect = 1.f;
    envmapping = true;

	physent *oldcamera = camera1;
	static physent cmcamera;
	cmcamera = *camera1;
	cmcamera.reset();
	cmcamera.type = ENT_CAMERA;
	cmcamera.o = o;
	cmcamera.yaw = yaw;
	cmcamera.pitch = pitch;
	cmcamera.roll = 0;
	camera1 = &cmcamera;

	defaultshader->set();

    updatedynlights();

    int fogmat = lookupmaterial(camera1->o)&MATF_VOLUME, abovemat = MAT_AIR;
    float fogblend = 1.0f, causticspass = 0.0f;
    if(fogmat==MAT_WATER || fogmat==MAT_LAVA)
    {
        float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
        else fogmat = abovemat;
        if(level && caustics && fogmat==MAT_WATER && camera1->o.z < z)
            causticspass = renderpath==R_FIXEDFUNCTION ? 1.0f : min(z - camera1->o.z, 1.0f);
    }
    else
    {
    	fogmat = MAT_AIR;
    }
    setfog(fogmat, fogblend, abovemat);
    if(level && fogmat != MAT_AIR)
    {
        float blend = abovemat==MAT_AIR ? fogblend : 1.0f;
        fovy += blend*sinf(lastmillis/1000.0)*2.0f;
        aspect += blend*sinf(lastmillis/1000.0+PI)*0.1f;
    }

	int farplane = hdr.worldsize*2;

    project(fovy, aspect, farplane, flipx, flipy, swapxy);
	transplayer();

	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	xtravertsva = xtraverts = glde = gbatches = 0;

    if(level && !hasFBO)
    {
        if(dopostfx)
        {
            drawglaretex();
            drawdepthfxtex();
            drawreflections();
        }
        else dopostfx = true;
    }

	visiblecubes(fovx, fovy);

	if(level && shadowmap && !hasFBO) rendershadowmap();

	glClear(GL_DEPTH_BUFFER_BIT);

	if(limitsky()) drawskybox(farplane, true);

	rendergeom(level ? causticspass : 0);

	if(level) queryreflections();

    if(!limitsky()) drawskybox(farplane, false);

	rendermapmodels();

	if(level >= 2)
	{
		rendergame();

	    if(hasFBO)
	    {
	        drawglaretex();
	        drawdepthfxtex();
	        drawreflections();
	    }

		renderdecals(0);
		renderportals(0);
	    renderwater();
		rendergrass();

		rendermaterials();
		render_particles(0);

		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);

	    addglare();
		glEnable(GL_CULL_FACE);
		glEnable(GL_FOG);
	}

	glDisable(GL_TEXTURE_2D);

	camera1 = oldcamera;
    envmapping = false;
}

VARP(scr_virtw, 0, 1024, INT_MAX-1);
VARP(scr_virth, 0, 768, INT_MAX-1);
VARP(scr_minw, 0, 640, INT_MAX-1);
VARP(scr_minh, 0, 480, INT_MAX-1);

void getscreenres(int &w, int &h)
{
    float wk = 1, hk = 1;
    if(w < scr_virtw) wk = float(scr_virtw)/w;
    if(h < scr_virth) hk = float(scr_virth)/h;
    wk = hk = max(wk, hk);
    w = int(ceil(w*wk));
    h = int(ceil(h*hk));
}

void gettextres(int &w, int &h)
{
	if(w < scr_minw || h < scr_minh)
	{
		if(scr_minw > w*scr_minh/h)
		{
			h = h*scr_minw/w;
			w = scr_minw;
		}
		else
		{
			w = w*scr_minh/h;
			h = scr_minh;
		}
	}
}

const char *loadback = "textures/loadback";

void loadbackground(int w, int h)
{
	glColor3f(1, 1, 1);

	settexture(loadback);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(1, 0); glVertex2f(w, 0);
	glTexCoord2f(1, 1); glVertex2f(w, h);
	glTexCoord2f(0, 1); glVertex2f(0, h);

	glEnd();
}

void computescreen(const char *text, Texture *t, const char *overlaytext)
{
	int w = screen->w, h = screen->h;
	if(overlaytext && text)
    {
        s_sprintfd(caption)("%s - %s", overlaytext, text);
        setcaption(caption);
    }
    else setcaption(text);
    getscreenres(w, h);
	gettextres(w, h);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.f, 0.f, 0.f, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    defaultshader->set();
	loopi(2)
	{
		glClear(GL_COLOR_BUFFER_BIT);

		loadbackground(w, h);

        if(text)
        {
            glPushMatrix();
            glScalef(1/3.0f, 1/3.0f, 1);
            draw_text(text, 70, 2*FONTH + FONTH/2);
            glPopMatrix();
        }
		if(t)
		{
			glBindTexture(GL_TEXTURE_2D, t->id);
			int sz = 256, x = (w-sz)/2, y = min(384, h-256);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(x,	y);
			glTexCoord2f(1, 0); glVertex2f(x+sz, y);
			glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
			glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
			glEnd();
			settexture(guioverlaytex);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(x,	y);
			glTexCoord2f(1, 0); glVertex2f(x+sz, y);
			glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
			glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
			glEnd();
		}
        if(overlaytext)
        {
			int sz = 256, x = (w-sz)/2, y = min(384, h-256);
            glPushMatrix();
            glScalef(1/3.0f, 1/3.0f, 1);
			draw_textx("%s", x*3+sz*3-FONTH, y*3+FONTH, 255, 255, 255, 255, false, AL_RIGHT, -1, sz*3-FONTH*2, overlaytext);
            glPopMatrix();
        }

		glPushMatrix();
		glScalef(1/3.0f, 1/3.0f, 1);
		draw_textx("v%.2f (%s)", w*3-FONTH, int(h*2.6f), 255, 255, 255, 255, false, AL_RIGHT, -1, -1, float(ENG_VERSION)/100.f, ENG_RELEASE);
		glPopMatrix();

		int x = (w-512)/2, y = 128;
		settexture("textures/logo");
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(x,	 y);
		glTexCoord2f(1, 0); glVertex2f(x+512, y);
		glTexCoord2f(1, 1); glVertex2f(x+512, y+256);
		glTexCoord2f(0, 1); glVertex2f(x,	 y+256);
		glEnd();

		SDL_GL_SwapBuffers();
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

static void bar(float bar, int w, int o, float r, float g, float b)
{
	int side = 2*FONTH;
	float x1 = side, x2 = min(bar, 1.0f)*(w*3-2*side)+side;
	float y1 = o*FONTH;
	glColor3f(r, g, b);
	glBegin(GL_TRIANGLE_STRIP);
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
		glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	glEnd();

#if 0
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_LINE_LOOP);
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 - sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
	}
	glEnd();
#endif
}

void renderprogress(float bar1, const char *text1, float bar2, const char *text2, GLuint tex)	// also used during loading
{
	if(!inbetweenframes) return;
	clientkeepalive();

    #ifdef __APPLE__
    interceptkey(SDLK_UNKNOWN); // keep the event queue awake to avoid 'beachball' cursor
    #endif

	if(verbose >= 4)
	{
		if (text2) conoutf("\fm%s [%.2f%%], %s [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
		else if (text1) conoutf("\fm%s [%.2f%%]", text1, bar1*100.f);
		else conoutf("\fmprogressing [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
	}

	int w = screen->w, h = screen->h;
    getscreenres(w, h);
	gettextres(w, h);

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, w*3, h*3, 0, -1, 1);
	notextureshader->set();

	glLineWidth(3);

	if(text1)
	{
		bar(1, w, 4, 0, 0, 0.8f);
		if(bar1>0) bar(bar1, w, 4, 0, 0.5f, 1);
	}

	if(bar2>0)
	{
		bar(1, w, 6, 0.5f, 0, 0);
		bar(bar2, w, 6, 0.75f, 0, 0);
	}

	glLineWidth(1);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	defaultshader->set();

	if(text1) draw_text(text1, 2*FONTH, 4*FONTH + FONTH/2);
	if(bar2>0) draw_text(text2, 2*FONTH, 6*FONTH + FONTH/2);

	glDisable(GL_BLEND);

	if(tex)
	{
		glBindTexture(GL_TEXTURE_2D, tex);
		int sz = 256, x = (w-sz)/2, y = min(384, h-256);
		sz *= 3;
		x *= 3;
		y *= 3;
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(x,	y);
		glTexCoord2f(1, 0); glVertex2f(x+sz, y);
		glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
		glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	SDL_GL_SwapBuffers();
}

GLfloat mvmatrix[16], projmatrix[16], mvpmatrix[16], invmvmatrix[16];

void readmatrices()
{
    glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);

    loopi(4) loopj(4)
    {
        float c = 0;
        loopk(4) c += projmatrix[k*4 + j] * mvmatrix[i*4 + k];
        mvpmatrix[i*4 + j] = c;
    }

    loopi(3)
    {
        loopj(3) invmvmatrix[i*4 + j] = mvmatrix[i + j*4];
        invmvmatrix[i*4 + 3] = 0;
    }
    loopi(3)
    {
        float c = 0;
        loopj(3) c -= mvmatrix[i*4 + j] * mvmatrix[12 + j];
        invmvmatrix[12 + i] = c;
    }
    invmvmatrix[15] = 1;
}

VARP(hidehud, 0, 0, 1);
FVARP(hudblend, 0.99f);

float cursorx = 0.5f, cursory = 0.5f, aimx = 0.5f, aimy = 0.5f;
vec cursordir(0, 0, 0);

struct framebuffercopy
{
    GLuint tex;
    GLenum target;
    int w, h;

    framebuffercopy() : tex(0), target(GL_TEXTURE_2D), w(0), h(0) {}

    void cleanup()
    {
        if(!tex) return;
        glDeleteTextures(1, &tex);
		tex = 0;
    }

    void setup()
    {
        if(tex) return;
        glGenTextures(1, &tex);
        if(hasTR)
        {
            target = GL_TEXTURE_RECTANGLE_ARB;
            w = screen->w;
            h = screen->h;
        }
        else
        {
            target = GL_TEXTURE_2D;
            for(w = 1; w < screen->w; w *= 2);
            for(h = 1; h < screen->h; h *= 2);
        }
        createtexture(tex, w, h, NULL, 3, false, GL_RGB, target);
    }

    void copy()
    {
        if(target == GL_TEXTURE_RECTANGLE_ARB)
        {
            if(w != screen->w || h != screen->h) cleanup();
        }
        else if(w < screen->w || h < screen->h || w/2 >= screen->w || h/2 >= screen->h) cleanup();
        if(!tex) setup();

        glBindTexture(target, tex);
        glCopyTexSubImage2D(target, 0, 0, 0, 0, 0, screen->w, screen->h);
    }

	void draw(float sx, float sy, float sw, float sh)
	{
		float tx = 0, ty = 0, tw = float(screen->w)/w, th = float(screen->h)/h;
		if(target == GL_TEXTURE_RECTANGLE_ARB)
		{
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			rectshader->set();
			tx *= w;
			ty *= h;
			tw *= w;
			th *= h;
		}
        glBindTexture(target, tex);
        glBegin(GL_QUADS);
        glTexCoord2f(tx,    ty);    glVertex2f(sx,    sy);
        glTexCoord2f(tx+tw, ty);    glVertex2f(sx+sw, sy);
        glTexCoord2f(tx+tw, ty+th); glVertex2f(sx+sw, sy+sh);
        glTexCoord2f(tx,    ty+th); glVertex2f(sx,    sy+sh);
        glEnd();
		if(target == GL_TEXTURE_RECTANGLE_ARB)
		{
			glEnable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
			defaultshader->set();
		}
	}
};

#define DTR 0.0174532925
enum { VW_NORMAL = 0, VW_MAGIC, VW_STEREO, VW_STEREO_BLEND = VW_STEREO, VW_STEREO_BLEND_REDCYAN, VW_STEREO_AVG, VW_MAX, VW_STEREO_REDCYAN = VW_MAX };
enum { VP_LEFT, VP_RIGHT, VP_MAX, VP_CAMERA = VP_MAX };

framebuffercopy views[VP_MAX];

VARFP(viewtype, VW_NORMAL, VW_NORMAL, VW_MAX, loopi(VP_MAX) views[i].cleanup());
VARP(stereoblend, 0, 50, 100);
FVARP(stereodist, 0.5f);
FVARP(stereoplane, 10.f);
FVARP(stereonear, 3.f);

int fogmat = MAT_AIR, abovemat = MAT_AIR;
float fogblend = 1.0f, causticspass = 0.0f;

GLenum colormask[3] = { GL_TRUE, GL_TRUE, GL_TRUE };

void setcolormask(bool r, bool g, bool b)
{
	colormask[0] = r ? GL_TRUE : GL_FALSE;
	colormask[1] = g ? GL_TRUE : GL_FALSE;
	colormask[2] = b ? GL_TRUE : GL_FALSE;
}

bool needsview(int v, int targtype)
{
    switch(v)
    {
        case VW_NORMAL: return targtype == VP_CAMERA;
        case VW_MAGIC: return targtype == VP_LEFT || targtype == VP_RIGHT;
        case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN: return targtype >= VP_LEFT && targtype <= VP_CAMERA;
        case VW_STEREO_AVG:
        case VW_STEREO_REDCYAN: return targtype == VP_LEFT || targtype == VP_RIGHT;
    }
    return false;
}

bool copyview(int v, int targtype)
{
    switch(v)
    {
        case VW_MAGIC: return targtype == VP_LEFT || targtype == VP_RIGHT;
        case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN: return targtype == VP_RIGHT;
        case VW_STEREO_AVG: return targtype == VP_LEFT;
    }
    return false;
}

bool clearview(int v, int targtype)
{
    switch(v)
    {
        case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN: return targtype == VP_LEFT || targtype == VP_CAMERA;
        case VW_STEREO_REDCYAN: return targtype == VP_LEFT;
    }
    return true;
}

static int curview = VP_CAMERA;

void viewproject(float zscale)
{
    if(curview != VP_LEFT && curview != VP_RIGHT) project(fovy, aspect, farplane, false, false, false, zscale);
    else
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if(zscale != 1) glScalef(1, 1, zscale);
        float top = stereonear*tan(DTR*fovy/2), right = aspect*top, iod = stereodist/2, fs = iod*stereonear/stereoplane;
        glFrustum(curview == VP_LEFT ? -right+fs : -right-fs, curview == VP_LEFT ? right+fs : right-fs, -top, top, stereonear, farplane);
        glTranslatef(curview == VP_LEFT ? iod : -iod, 0.f, 0.f);
        glMatrixMode(GL_MODELVIEW);
    }
}

void drawview(int targtype)
{
    curview = targtype;

	defaultshader->set();
	updatedynlights();

	setfog(fogmat, fogblend, abovemat);
  	viewproject();
	transplayer();
	if(targtype == VP_LEFT || targtype == VP_RIGHT)
	{
		if(viewtype >= VW_STEREO)
        {
            switch(viewtype)
            {
                case VW_STEREO_BLEND: setcolormask(targtype == VP_LEFT, false, targtype == VP_RIGHT); break;
                case VW_STEREO_AVG: setcolormask(targtype == VP_LEFT, true, targtype == VP_RIGHT); break;
                case VW_STEREO_BLEND_REDCYAN:
                case VW_STEREO_REDCYAN: setcolormask(targtype == VP_LEFT, targtype == VP_RIGHT, targtype == VP_RIGHT); break;
            }
            glColorMask(COLORMASK, GL_TRUE);
        }
	}

	readmatrices();

	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, wireframe && editmode ? GL_LINE : GL_FILL);

	xtravertsva = xtraverts = glde = gbatches = 0;

	if(!hasFBO)
	{
		if(dopostfx)
		{
			drawglaretex();
			drawdepthfxtex();
			drawreflections();
		}
		else dopostfx = true;
	}

	visiblecubes(curfov, fovy);
	if(shadowmap && !hasFBO) rendershadowmap();

	glClearColor(0, 0, 0, 0);
	glClear(GL_DEPTH_BUFFER_BIT|(wireframe && editmode && clearview(viewtype, targtype) ? GL_COLOR_BUFFER_BIT : 0)|(hasstencil ? GL_STENCIL_BUFFER_BIT : 0));

	if(limitsky()) drawskybox(farplane, true);
	rendergeom(causticspass);
	extern int outline, blankgeom;
	if(!wireframe && editmode && (outline || (fullbright && blankgeom))) renderoutline();
	queryreflections();
	if(!limitsky()) drawskybox(farplane, false);

	rendermapmodels();
	rendergame();
	renderavatar(true);

	if(hasFBO)
	{
		drawglaretex();
		drawdepthfxtex();
		drawreflections();
	}

	renderdecals(curtime);
	renderportals(curtime);
	renderwater();
	rendergrass();

	rendermaterials();
	render_particles(curtime);

    viewproject(0.5f);
	renderavatar(false);
    viewproject();

	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);

	addglare();
	renderfullscreenshader(screen->w, screen->h);

	glDisable(GL_TEXTURE_2D);
	notextureshader->set();
	if(editmode && !hidehud)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDepthMask(GL_FALSE);
		cursorupdate();
		glDepthMask(GL_TRUE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	int w = screen->w, h = screen->h;
	gettextres(w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glEnable(GL_BLEND);
	vec colour;
	bool hashudcolour = cl->gethudcolour(colour);
	if(hashudcolour || fogmat==MAT_WATER || fogmat==MAT_LAVA)
	{
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		if(hashudcolour) glColor3f(colour.x, colour.y, colour.z);
		else
		{
			float overlay[3] = { 0, 0, 0 };
			blendfogoverlay(fogmat, fogblend, overlay);
			blendfogoverlay(abovemat, 1-fogblend, overlay);
			glColor3fv(overlay);
		}
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(w, 0);
		glVertex2f(w, h);
		glVertex2f(0, h);
		glEnd();
	}
	glDisable(GL_BLEND);

	glColor3f(1, 1, 1);
	extern int debugsm;
	if(debugsm)
	{
		extern void viewshadowmap();
		viewshadowmap();
	}

	extern int debugglare;
	if(debugglare)
	{
		extern void viewglaretex();
		viewglaretex();
	}

	extern int debugdepthfx;
	if(debugdepthfx)
	{
		extern void viewdepthfxtex();
		viewdepthfxtex();
	}

	glEnable(GL_TEXTURE_2D);
	defaultshader->set();
	cl->drawhud(w, h);
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_FOG);
	renderedgame = false;

    if(targtype == VP_LEFT || targtype == VP_RIGHT)
    {
        if(viewtype >= VW_STEREO)
        {
            setcolormask();
            glColorMask(COLORMASK, GL_TRUE);
        }
    }
}

void gl_drawframe(int w, int h)
{
	fogmat = lookupmaterial(camera1->o)&MATF_VOLUME;
	if(fogmat == MAT_WATER || fogmat == MAT_LAVA)
	{
		float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
		if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
		else fogmat = abovemat;
		if(caustics && fogmat == MAT_WATER && camera1->o.z < z)
			causticspass = renderpath==R_FIXEDFUNCTION ? 1.0f : min(z - camera1->o.z, 1.0f);

		float blend = abovemat == MAT_AIR ? fogblend : 1.0f;
		fovy += blend*sinf(lastmillis/1000.0)*2.0f;
		aspect += blend*sinf(lastmillis/1000.0+PI)*0.1f;
	}
	else fogmat = MAT_AIR;

	farplane = hdr.worldsize*2;
	project(fovy, aspect, farplane);
	transplayer();
	readmatrices();
	cl->project(w, h);

	int copies = 0, oldcurtime = curtime;
	loopi(VP_MAX) if(needsview(viewtype, i))
	{
		drawview(i);
		if(copyview(viewtype, i))
		{
			views[i].copy();
			copies++;
		}
		curtime = 0;
	}
	if(needsview(viewtype, VP_CAMERA)) drawview(VP_CAMERA);
	curtime = oldcurtime;

	if(!copies) return;

	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	defaultshader->set();
	glColor3f(1.f, 1.f, 1.f);
	switch(viewtype)
	{
		case VW_MAGIC:
		{
			views[VP_LEFT].draw(0, 0, 0.5f, 1);
			views[VP_RIGHT].draw(0.5f, 0, 0.5f, 1);
			break;
		}
		case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN:
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if(viewtype == VW_STEREO_BLEND) glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
			glColor4f(1.f, 1.f, 1.f, stereoblend/100.f); views[VP_RIGHT].draw(0, 0, 1, 1);
            if(viewtype == VW_STEREO_BLEND) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDisable(GL_BLEND);
			break;
		}
        case VW_STEREO_AVG:
        {
            glEnable(GL_BLEND);
            if(hasBC)
            {
                glBlendFunc(GL_ONE, GL_CONSTANT_COLOR_EXT);
                glBlendColor_(0.f, 0.5f, 1.f, 1.f);
            }
            else
            {
                glDisable(GL_TEXTURE_2D);
                glBlendFunc(GL_ZERO, GL_SRC_COLOR);
                glColor3f(0.f, 0.5f, 1.f);
                glBegin(GL_QUADS);
                glVertex2f(0, 0);
                glVertex2f(1, 0);
                glVertex2f(1, 1);
                glVertex2f(0, 1);
                glEnd();
                glEnable(GL_TEXTURE_2D);
                glBlendFunc(GL_ONE, GL_ONE);
            }
            glColor3f(1.f, 0.5f, 0.f);
            views[VP_LEFT].draw(0, 0, 1, 1);
            glDisable(GL_BLEND);
            break;
        }
	}
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_FOG);
}

#define rendernearfar(a,b,c,d,e) \
	if (d) { \
		loopj(2) { \
			if (!j) { glDepthFunc(GL_GREATER); glColor3f(a*0.25f, b*0.25f, c*0.25f); } \
			else { glDepthFunc(GL_LESS); glColor3f(a, b, c); } \
			e; \
		} \
	} else { \
		glColor3f(a, b, c); \
		e; \
	}


void renderprimitive(bool on)
{
	if (on)
	{
		notextureshader->set();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else
	{
		defaultshader->set();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}
}

void renderline(vec &fr, vec &to, float r, float g, float b, bool nf)
{
	rendernearfar(r, g, b, nf,
	{
		glBegin(GL_LINES);
		glVertex3f(fr.x, fr.y, fr.z);
		glVertex3f(to.x, to.y, to.z);
		glEnd();
		xtraverts += 2;
	});
}

void rendertris(vec &fr, float yaw, float pitch, float size, float r, float g, float b, bool fill, bool nf)
{
	rendernearfar(r, g, b, nf,
	{
		vec to;
		float ty;

		glBegin(GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, fill ? GL_FILL : GL_LINE);

		glVertex3f(fr.x, fr.y, fr.z);

		ty = yaw - 45.f;
		if (ty < 0.f) ty += 360.f;
		else if (ty >= 360.f) ty -= 360.f;

		vecfromyawpitch(ty, pitch, -1, 0, to);
		to.mul(size);
		to.add(fr);
		glVertex3f(to.x, to.y, to.z);

		ty = yaw + 45.f;
		if (ty < 0.f) ty += 360.f;
		else if (ty >= 360.f) ty -= 360.f;

		vecfromyawpitch(ty, pitch, -1, 0, to);
		to.mul(size);
		to.add(fr);
		glVertex3f(to.x, to.y, to.z);

		glEnd();
		xtraverts += 3;
	});
}

void renderlineloop(vec &o, float xradius, float yradius, float zradius, float z, int type, float r, float g, float b, bool nf)
{
	rendernearfar(r, g, b, nf,
	{
		glBegin(GL_LINE_LOOP);
		loopi(16)
		{
			vec p;
			switch (type)
			{
				case 0:
					p = vec(xradius*cosf(2*M_PI*i/16.0f), zradius*sinf(2*M_PI*i/16.0f), 0);
					p.rotate_around_x((z+90)*RAD);
					break;
				case 1:
					p = vec(zradius*cosf(2*M_PI*i/16.0f), yradius*sinf(2*M_PI*i/16.0f), 0);
					p.rotate_around_y((z+90)*RAD);
					break;
				case 2:
				default:
					p = vec(xradius*cosf(2*M_PI*i/16.0f), yradius*sinf(2*M_PI*i/16.0f), 0);
					p.rotate_around_z((z+90)*RAD);
					break;
			}
			p.add(o);
			glVertex3fv(p.v);
		}
		xtraverts += 16;
		glEnd();
	});
}

void renderdir(vec &o, float yaw, float pitch, bool nf)
{
	vec fr = o, to, dr;

	vecfromyawpitch(yaw, pitch, 1, 0, dr);

	to = dr;
	to.mul(RENDERPUSHX);
	to.add(fr);
	fr.z += RENDERPUSHZ;
	to.z += RENDERPUSHZ;

	renderline(fr, to, 0.f, 0.f, 1.f, nf);

	dr.mul(0.5f);
	to.add(dr);
	rendertris(to, yaw, pitch, 2.f, 0.f, 0.f, 0.5f, true, nf);
}

void renderradius(vec &o, float xradius, float yradius, float zradius, bool nf)
{
	renderlineloop(o, xradius, yradius, zradius, 0.f, 0, 0.f, 0.75, 0.75f, nf);
	renderlineloop(o, xradius, yradius, zradius, 0.f, 1, 0.f, 0.5f, 1.f, nf);
	renderlineloop(o, xradius, yradius, zradius, 0.f, 2, 0.f, 1.f, 0.5f, nf);
}

bool rendericon(const char *icon, int x, int y, int xs, int ys)
{
	Texture *t;

	if ((t = textureload(icon, 0, true, false)) != notexture)
	{
		glBindTexture(GL_TEXTURE_2D, t->id);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+xs, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+xs, y+ys);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+ys);
		glEnd();
		return true;
	}
	return false;
}

vector<portal *> portals;

vec portalcolours[PORTAL_MAX] = { vec(0.f, 0.f, 1.f), vec(1.f, 0.5f, 0.f) };
Texture *portaltexture = NULL;
TVARN(portaltex, "textures/portal", portaltexture, 0);

void renderportals(int time)
{
	if(!portaltexture || portaltexture==notexture)
		if(!(portaltexture = textureload(portaltex, 0, true)))
			fatal("could not load portal texture");

	loopv(portals)
	{
		portal *p = portals[i];
		glPushMatrix();
		vec o(vec(p->o).add(p->n));
		glTranslatef(o.x, o.y, o.z);
		glRotatef(p->yaw-180.f, 0, 0, 1);
		glRotatef(p->pitch, 1, 0, 0);
		glScalef(p->radius, p->radius, p->radius);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		if(portaltexture->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		glColor3fv(portalcolours[p->type].v);
		glBindTexture(GL_TEXTURE_2D, portaltexture->id);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.f, 0.f, 1.f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(1.f, 0.f, 1.f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(1.f, 0.f, -1.f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.f, 0.f, -1.f);
		xtraverts += 4;
		glEnd();
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glPopMatrix();
	}
}
