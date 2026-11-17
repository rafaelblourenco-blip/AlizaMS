#include "CG/shaders_core.h"
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMatrix4x4>
#include <QVector4D>
#include <iostream>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QSurfaceFormat fmt; fmt.setRenderableType(QSurfaceFormat::OpenGL); fmt.setVersion(3,2); fmt.setProfile(QSurfaceFormat::CoreProfile);
    QOpenGLContext context; context.setFormat(fmt);
    if (!context.create()) return 77;
    QOffscreenSurface surface; surface.setFormat(context.format()); surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface)) return 77;
    auto *gl = context.versionFunctions<QOpenGLFunctions_3_2_Core>();
    if (!gl || !gl->initializeOpenGLFunctions()) return 77;
    const char *slices[] = {c3d_fs,c3d_fs_clamp,c3d_fs_bb,c3d_fs_bb_clamp,c3d_fs_gradient,c3d_fs_gradient_clamp,c3d_fs_gradient_bb,c3d_fs_gradient_bb_clamp,c3d_fs_sigm,c3d_fs_clamp_sigm,c3d_fs_bb_sigm,c3d_fs_bb_clamp_sigm,c3d_fs_gradient_sigm,c3d_fs_gradient_clamp_sigm,c3d_fs_gradient_bb_sigm,c3d_fs_gradient_bb_clamp_sigm};
    const char *rays[] = {raycast_fs,raycast_fs_bb,raycast_color_fs,raycast_color_fs_bb,raycast_fs_sigm,raycast_fs_bb_sigm,raycast_color_fs_sigm,raycast_color_fs_bb_sigm,mip_fs,mip_fs_bb,mip_color_fs,mip_color_fs_bb,mip_fs_sigm,mip_fs_bb_sigm,mip_color_fs_sigm,mip_color_fs_bb_sigm};
    for (int i=0; i<32; ++i) {
        QOpenGLShaderProgram p;
        if (!p.addShaderFromSourceCode(QOpenGLShader::Vertex,i<16?c3d_vs:raycast_vs) ||
            !p.addShaderFromSourceCode(QOpenGLShader::Fragment,i<16?slices[i]:rays[i-16]) || !p.link()) {
            std::cerr << "shader " << i << ": " << p.log().toStdString() << '\n'; return 1;
        }
        if (p.uniformLocation("sculptMask") < 0 || p.uniformLocation("sculptEnabled") < 0) {
            std::cerr << "sculpt uniforms absent from shader " << i << '\n'; return 1;
        }
    }
    QOpenGLShaderProgram p;
    if (!p.addShaderFromSourceCode(QOpenGLShader::Vertex,c3d_vs) || !p.addShaderFromSourceCode(QOpenGLShader::Fragment,c3d_fs) || !p.link() || !p.bind()) return 1;
    QOpenGLFramebufferObject fbo(2,1);
    if (!fbo.isValid() || !fbo.bind()) return 1;
    gl->glViewport(0,0,2,1); gl->glDisable(GL_BLEND); gl->glClearColor(0,0,0,0); gl->glClear(GL_COLOR_BUFFER_BIT);
    const GLfloat vertices[] = {-1,-1,0, 0,0,0.5, 1,-1,0, 1,0,0.5, -1,1,0, 0,1,0.5, 1,1,0, 1,1,0.5};
    GLuint vao=0,vbo=0,volume=0,mask=0;
    gl->glGenVertexArrays(1,&vao); gl->glBindVertexArray(vao); gl->glGenBuffers(1,&vbo); gl->glBindBuffer(GL_ARRAY_BUFFER,vbo);
    gl->glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    const int pos=p.attributeLocation("v_position"), tc=p.attributeLocation("v_texcoord0");
    if (pos<0 || tc<0) return 1;
    gl->glEnableVertexAttribArray(pos); gl->glVertexAttribPointer(pos,3,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),nullptr);
    gl->glEnableVertexAttribArray(tc); gl->glVertexAttribPointer(tc,3,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),reinterpret_cast<void*>(3*sizeof(GLfloat)));
    const GLfloat voxels[]={0.5f,0.5f}; const GLubyte bits[]={0,255};
    auto makeTexture=[&](GLuint &id, GLenum unit, GLint internal, GLenum format, GLenum type, const void *data) {
        gl->glGenTextures(1,&id); gl->glActiveTexture(unit); gl->glBindTexture(GL_TEXTURE_3D,id);
        gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
        gl->glPixelStorei(GL_UNPACK_ALIGNMENT,1); gl->glTexImage3D(GL_TEXTURE_3D,0,internal,2,1,1,0,format,type,data);
    };
    makeTexture(volume,GL_TEXTURE0,GL_R32F,GL_RED,GL_FLOAT,voxels);
    makeTexture(mask,GL_TEXTURE7,GL_R8,GL_RED,GL_UNSIGNED_BYTE,bits);
    p.setUniformValue("sampler0",0); p.setUniformValue("sculptMask",7); p.setUniformValue("sculptEnabled",1);
    QMatrix4x4 identity; p.setUniformValue("mvp",identity);
    const QVector4D params[]={QVector4D(2,0,1,1),QVector4D(1,0,1,0),QVector4D(1,0,1,1),QVector4D(1,1,0.5,0)};
    p.setUniformValueArray("mparams",params,4);
    gl->glDrawArrays(GL_TRIANGLE_STRIP,0,4); GLubyte pixels[8]={}; gl->glReadPixels(0,0,2,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    if (gl->glGetError()!=GL_NO_ERROR || pixels[0]!=0 || pixels[4]<100 || pixels[4]>155) {
        std::cerr<<"mask draw failed: "<<int(pixels[0])<<", "<<int(pixels[4])<<'\n'; return 1;
    }
    // Exercise the ray traversal and MIP paths using two synthetic rays.
    GLuint endpoints[2] = {};
    gl->glGenTextures(2,endpoints);
    const GLfloat back[] = {0.25f,0.5f,0.0f, 0.75f,0.5f,0.0f};
    const GLfloat front[] = {0.25f,0.5f,1.0f, 0.75f,0.5f,1.0f};
    for (int i=0;i<2;++i) {
        gl->glActiveTexture(i == 0 ? GL_TEXTURE2 : GL_TEXTURE5);
        gl->glBindTexture(GL_TEXTURE_2D,endpoints[i]);
        gl->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        gl->glTexImage2D(GL_TEXTURE_2D,0,GL_RGB32F,2,1,0,GL_RGB,GL_FLOAT,i == 0 ? back : front);
    }
    for (const char *fragment : {raycast_fs,mip_fs}) {
        QOpenGLShaderProgram ray;
        if (!ray.addShaderFromSourceCode(QOpenGLShader::Vertex,raycast_vs) ||
            !ray.addShaderFromSourceCode(QOpenGLShader::Fragment,fragment) || !ray.link() || !ray.bind()) return 1;
        gl->glClear(GL_COLOR_BUFFER_BIT);
        const int rayPos=ray.attributeLocation("v_position");
        gl->glEnableVertexAttribArray(rayPos);
        gl->glVertexAttribPointer(rayPos,3,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),nullptr);
        ray.setUniformValue("mvp",identity);
        const QVector4D rayParams[]={QVector4D(2,0,1,1),QVector4D(2,0,1,0),QVector4D(1,0,1,1),QVector4D(1,1,0.5,0)};
        ray.setUniformValueArray("mparams",rayParams,4);
        ray.setUniformValue("sampler0",2); ray.setUniformValue("sampler1",5); ray.setUniformValue("sampler2",0);
        ray.setUniformValue("sculptMask",7); ray.setUniformValue("sculptEnabled",1);
        gl->glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        gl->glReadPixels(0,0,2,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        if (gl->glGetError()!=GL_NO_ERROR || pixels[0]!=0 || pixels[4]<20) {
            std::cerr<<"ray mask draw failed: "<<int(pixels[0])<<", "<<int(pixels[4])<<'\n'; return 1;
        }
    }
    gl->glDeleteTextures(2,endpoints);
    gl->glDeleteTextures(1,&volume); gl->glDeleteTextures(1,&mask); gl->glDeleteBuffers(1,&vbo); gl->glDeleteVertexArrays(1,&vao);
    std::cout<<"32 shaders compiled; slice, raycast and MIP masks retained only expected pixels\n";
    return 0;
}
