#ifndef ALIZA_MPR_GPU_RENDERER_H
#define ALIZA_MPR_GPU_RENDERER_H

#include "common/mprgpugeometry.h"
#include "common/mprgpushader.h"
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0) && defined(ALIZA_GL_3_2_CORE)
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QVector3D>
#include <itkWeakPointer.h>
#include <vector>
#include <limits>
#include <memory>

// GUI-thread renderer. The scalar framebuffer is read back for the existing
// pixmap, measurement and export pipeline; it is not yet a zero-copy viewport.
class MprGpuRenderer {
    using Projection = mpr_slab::ProjectionImage;
    QOpenGLContext *context_ = nullptr;
    QOffscreenSurface *surface_ = nullptr;
    QOpenGLFunctions_3_2_Core *gl_ = nullptr;
    QOpenGLShaderProgram *shader_ = nullptr;
    std::unique_ptr<QOpenGLFramebufferObject> framebuffer_;
    GLuint texture_ = 0, vao_ = 0;
    itk::WeakPointer<const itk::DataObject> cached_;
    unsigned long cached_mtime_ = 0;
    int dimensions_[3] = {0,0,0};
    bool attempted_ = false, ready_ = false;
    struct ContextGuard {
        QOpenGLContext *old = QOpenGLContext::currentContext();
        QSurface *surface = old ? old->surface() : nullptr;
        QOpenGLContext *next;
        bool current;
        ContextGuard(QOpenGLContext *c, QOffscreenSurface *s) : next(c), current(c && s && c->makeCurrent(s)) {}
        ~ContextGuard() { if (current) { next->doneCurrent(); if (old && surface) old->makeCurrent(surface); } }
    };
    bool initialize() {
        if (attempted_) return ready_;
        attempted_ = true;
        if (qApp && qApp->property("aliza_disable_opengl").toBool()) return false;
        QSurfaceFormat fmt; fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setVersion(3,2); fmt.setProfile(QSurfaceFormat::CoreProfile);
        context_ = new QOpenGLContext(); context_->setFormat(fmt);
        if (QOpenGLContext::globalShareContext()) context_->setShareContext(QOpenGLContext::globalShareContext());
        if (!context_->create()) return false;
        surface_ = new QOffscreenSurface(); surface_->setFormat(context_->format()); surface_->create();
        ContextGuard guard(context_,surface_); if (!surface_->isValid() || !guard.current) return false;
        gl_ = context_->versionFunctions<QOpenGLFunctions_3_2_Core>();
        if (!gl_ || !gl_->initializeOpenGLFunctions()) return false;
        shader_ = new QOpenGLShaderProgram();
        if (!shader_->addShaderFromSourceCode(QOpenGLShader::Vertex,mpr_gpu::vertex_shader) ||
            !shader_->addShaderFromSourceCode(QOpenGLShader::Fragment,mpr_gpu::projection_shader) || !shader_->link()) return false;
        gl_->glGenVertexArrays(1,&vao_);
        ready_ = vao_ != 0 && gl_->glGetError()==GL_NO_ERROR;
        return ready_;
    }
    template<typename Image> bool upload(const Image *input) {
        if (cached_.GetPointer()==input && cached_mtime_==input->GetMTime() && texture_) return true;
        const auto region=input->GetLargestPossibleRegion();
        if (input->GetBufferedRegion()!=region || !input->GetBufferPointer()) return false;
        const auto size=region.GetSize(); GLint limit=0; gl_->glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE,&limit);
        size_t count=1;
        for (int i=0;i<3;++i) {
            if (!size[i] || size[i]>static_cast<size_t>(limit) || count>size_t(268435456)/size[i]) return false;
            count*=size[i];
        }
        std::vector<float> pixels;
        try { pixels.resize(count); } catch (...) { return false; }
        // R32F represents every integer up to 2^24 and every finite float.
        // Reject other values rather than silently changing the data.
        for (size_t i=0;i<count;++i) {
            const double value=static_cast<double>(input->GetBufferPointer()[i]);
            if (!std::isfinite(value) ||
                (std::numeric_limits<typename Image::PixelType>::is_integer && std::abs(value)>16777216.0)) return false;
            pixels[i]=static_cast<float>(value);
        }
        GLuint fresh=0; while (gl_->glGetError()!=GL_NO_ERROR) {}
        gl_->glGenTextures(1,&fresh); gl_->glBindTexture(GL_TEXTURE_3D,fresh);
        gl_->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        gl_->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        gl_->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        gl_->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        gl_->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
        gl_->glPixelStorei(GL_UNPACK_ALIGNMENT,4);
        gl_->glTexImage3D(GL_TEXTURE_3D,0,GL_R32F,GLsizei(size[0]),GLsizei(size[1]),GLsizei(size[2]),0,GL_RED,GL_FLOAT,pixels.data());
        if (!fresh || gl_->glGetError()!=GL_NO_ERROR) { if (fresh) gl_->glDeleteTextures(1,&fresh); return false; }
        if (texture_) gl_->glDeleteTextures(1,&texture_);
        texture_=fresh; cached_=input; cached_mtime_=input->GetMTime();
        for (int i=0;i<3;++i) dimensions_[i]=static_cast<int>(size[i]);
        return true;
    }
public:
    static MprGpuRenderer &instance() { static MprGpuRenderer *renderer=new MprGpuRenderer; return *renderer; }
    // Call when a view is cleared or changes volume. Cached input is weakly
    // referenced, so it cannot keep the study alive.
    void releaseVolume() {
        if (!ready_ || !texture_) return;
        ContextGuard guard(context_,surface_);
        if (!guard.current) return;
        gl_->glDeleteTextures(1,&texture_); texture_=0; cached_=nullptr; cached_mtime_=0;
        framebuffer_.reset();
    }
    template<typename Image>
    Projection render(const Image *input, mpr_geometry::Vector3 c, mpr_geometry::Vector3 u,
                      mpr_geometry::Vector3 v, unsigned width, unsigned height, double su, double sv,
                      double thickness, int mode) {
        // Float and integers up to 32 bits are eligible. Wider types stay
        // on CPU; upload() validates the actual range and finiteness.
        if (std::numeric_limits<typename Image::PixelType>::digits > 32 &&
            !(std::numeric_limits<typename Image::PixelType>::is_iec559 &&
              std::numeric_limits<typename Image::PixelType>::digits == 24)) return {};
        if (!std::numeric_limits<typename Image::PixelType>::is_integer &&
            std::numeric_limits<typename Image::PixelType>::digits != 24) return {};
        if (!input || !width || !height || width>4096 || height>4096 ||
            static_cast<double>(width)*height>4194304 || !initialize()) return {};
        mpr_gpu::SamplingGeometry g;
        if (!mpr_gpu::sampling_geometry(input,c,u,v,width,height,su,sv,thickness,g) || g.half_samples>2048 ||
            double(width)*height*(2.0*g.half_samples+1.0)>30000000.0) return {};
        ContextGuard guard(context_,surface_); if (!guard.current || !upload(input)) return {};
        if (!framebuffer_ || framebuffer_->width()!=int(width) || framebuffer_->height()!=int(height)) {
            framebuffer_.reset(); QOpenGLFramebufferObjectFormat f; f.setInternalTextureFormat(GL_RG32F);
            framebuffer_.reset(new QOpenGLFramebufferObject(int(width),int(height),f));
        }
        if (!framebuffer_->isValid() || !framebuffer_->bind() || !shader_->bind()) return {};
        gl_->glViewport(0,0,GLsizei(width),GLsizei(height)); gl_->glDisable(GL_BLEND);
        gl_->glActiveTexture(GL_TEXTURE0); gl_->glBindTexture(GL_TEXTURE_3D,texture_); gl_->glBindVertexArray(vao_);
        auto vec=[&](const char *name,mpr_geometry::Vector3 a) { shader_->setUniformValue(name,QVector3D(float(a.x),float(a.y),float(a.z))); };
        shader_->setUniformValue("volume",0); shader_->setUniformValue("volume_size",QVector3D(float(dimensions_[0]),float(dimensions_[1]),float(dimensions_[2])));
        vec("first_index",g.first); vec("index_du",g.du); vec("index_dv",g.dv); vec("normal_per_mm",g.normal_per_mm);
        shader_->setUniformValue("sample_step_mm",float(g.sample_step)); shader_->setUniformValue("half_samples",g.half_samples);
        shader_->setUniformValue("mode",mode==0 ? 3 : mode);
        gl_->glDrawArrays(GL_TRIANGLES,0,3);
        std::vector<float> values;
        try { values.resize(size_t(width)*height*2); } catch (...) { framebuffer_->release(); return {}; }
        gl_->glReadPixels(0,0,GLsizei(width),GLsizei(height),GL_RG,GL_FLOAT,values.data());
        const GLenum err=gl_->glGetError(); shader_->release(); framebuffer_->release(); if(err!=GL_NO_ERROR) return {};
        using Output=itk::Image<double,2>; using Mask=itk::Image<unsigned char,2>;
        Output::IndexType start; start.Fill(0); Output::SizeType size; size[0]=width; size[1]=height;
        Output::RegionType region; region.SetIndex(start); region.SetSize(size);
        auto pixels=Output::New(); pixels->SetRegions(region); Output::SpacingType spacing; spacing[0]=su; spacing[1]=sv; pixels->SetSpacing(spacing); pixels->Allocate();
        auto mask=Mask::New(); mask->SetRegions(region); mask->SetSpacing(spacing); mask->Allocate();
        bool all=true; for (size_t i=0;i<size_t(width)*height;++i) {
            bool valid=values[2*i+1]>0.5f; pixels->GetBufferPointer()[i]=valid ? values[2*i] : 0.;
            mask->GetBufferPointer()[i]=valid ? 1 : 0; all=all && valid;
        }
        Projection result; result.pixels=pixels; result.valid=mask; result.all_valid=all; return result;
    }
};
#endif
#endif
