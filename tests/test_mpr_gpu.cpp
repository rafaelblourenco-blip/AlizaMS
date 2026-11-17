#include "../common/mprgpugeometry.h"
#include "../common/mprgpushader.h"
#define ALIZA_GL_3_2_CORE
#include "../GUI/mprgpurenderer.h"
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
using Image=itk::Image<short,3>;
template<typename Pixel>
bool test_scalar_type(MprGpuRenderer &renderer, const char *label) {
    using ScalarImage=itk::Image<Pixel,3>;
    auto img=ScalarImage::New(); typename ScalarImage::IndexType start; start.Fill(0);
    typename ScalarImage::SizeType size; size.Fill(3);
    typename ScalarImage::RegionType region; region.SetIndex(start); region.SetSize(size);
    img->SetRegions(region); img->Allocate();
    for (int z=0;z<3;++z) for (int y=0;y<3;++y) for (int x=0;x<3;++x) {
        typename ScalarImage::IndexType i; i[0]=x; i[1]=y; i[2]=z;
        img->SetPixel(i,static_cast<Pixel>(200+x*11+y*17+z*23));
    }
    const mpr_geometry::Vector3 c{1,1,1},u{1,0,0},v{0,1,0};
    for (double thickness : {0.0,2.0}) for (int mode : {1,2,3}) {
        auto gpu=renderer.render(img.GetPointer(),c,u,v,3,3,1,1,thickness,mode);
        auto cpu=mpr_slab::render_itk(img.GetPointer(),c,u,v,3,3,1,1,thickness,
            mode==1?mpr_slab::Mode::Maximum:(mode==2?mpr_slab::Mode::Minimum:mpr_slab::Mode::Mean));
        if(gpu.pixels.IsNull() || cpu.pixels.IsNull()) { std::cerr<<label<<" unavailable\n"; return false; }
        for(int y=0;y<3;++y) for(int x=0;x<3;++x) {
            itk::Index<2> i; i[0]=x; i[1]=y;
            if(gpu.valid->GetPixel(i)!=cpu.valid->GetPixel(i) ||
                std::abs(gpu.pixels->GetPixel(i)-cpu.pixels->GetPixel(i))>0.25) {
                std::cerr<<label<<" GPU/CPU mismatch\n"; return false;
            }
        }
    }
    return true;
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    if (app.arguments().contains("--benchmark")) {
        auto image=Image::New(); Image::IndexType start; start.Fill(0);
        Image::SizeType size; size.Fill(512); Image::RegionType region; region.SetIndex(start); region.SetSize(size);
        image->SetRegions(region); image->Allocate();
        for (std::size_t i=0; i<std::size_t(512)*512*512; ++i)
            image->GetBufferPointer()[i]=static_cast<short>(int((i*2654435761ULL >> 20) & 2047)-1024);
        mpr_geometry::Vector3 center{255.5,255.5,255.5};
        const mpr_geometry::Vector3 us[]={{1,0,0},{1,0,0},{0,1,0}};
        const mpr_geometry::Vector3 vs[]={{0,1,0},{0,0,1},{0,0,1}};
        auto &renderer=MprGpuRenderer::instance();
        const auto t0=std::chrono::steady_clock::now();
        auto initial=renderer.render(image.GetPointer(),center,us[0],vs[0],512,512,1,1,0,0);
        const auto t1=std::chrono::steady_clock::now();
        if(initial.pixels.IsNull()) { std::cerr<<"benchmark upload/initial render failed\n"; return 1; }
        std::cout<<"Initial upload + render ms: "<<std::chrono::duration<double,std::milli>(t1-t0).count()<<std::endl;
        for(int mode : {1,3}) for(double thickness : {5.0,10.0,20.0}) {
            for(int view=0;view<3;++view) {
                const auto begin=std::chrono::steady_clock::now();
                auto out=renderer.render(image.GetPointer(),center,us[view],vs[view],512,512,1,1,thickness,mode);
                const auto end=std::chrono::steady_clock::now();
                if(out.pixels.IsNull()) { std::cerr<<"benchmark render failed\n"; return 1; }
                std::cout<<"mode="<<mode<<" thickness="<<thickness<<" view="<<view<<" ms="<<std::chrono::duration<double,std::milli>(end-begin).count()<<std::endl;
            }
        }
        renderer.releaseVolume(); return 0;
    }
    if (app.arguments().contains("--disabled")) {
        app.setProperty("aliza_disable_opengl",true);
        auto image=Image::New(); Image::IndexType start; start.Fill(0);
        Image::SizeType size; size.Fill(1); Image::RegionType region; region.SetIndex(start); region.SetSize(size);
        image->SetRegions(region); image->Allocate(); image->FillBuffer(-42);
        auto output=MprGpuRenderer::instance().render(image.GetPointer(),{0,0,0},{1,0,0},{0,1,0},1,1,1,1,0,0);
        if (output.pixels.IsNotNull()) { std::cerr<<"disabled backend must reject request\n"; return 1; }
        auto fallback=mpr_slab::render_itk(image.GetPointer(),{0,0,0},{1,0,0},{0,1,0},1,1,1,1,0,mpr_slab::Mode::Mean);
        itk::Index<2> i; i.Fill(0);
        if (fallback.pixels.IsNull() || fallback.pixels->GetPixel(i)!=-42) { std::cerr<<"CPU fallback failed\n"; return 1; }
        std::cout<<"disabled GPU and CPU fallback passed\n"; return 0;
    }
    QSurfaceFormat fmt; fmt.setRenderableType(QSurfaceFormat::OpenGL); fmt.setVersion(3,2); fmt.setProfile(QSurfaceFormat::CoreProfile);
    QOpenGLContext context; context.setFormat(fmt);
    if (!context.create()) { std::cerr<<"OpenGL context unavailable\n"; return 77; }
    QOffscreenSurface surface; surface.setFormat(context.format()); surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface)) { std::cerr<<"OpenGL surface unavailable\n"; return 77; }
    auto *gl=context.versionFunctions<QOpenGLFunctions_3_2_Core>();
    if (!gl || !gl->initializeOpenGLFunctions()) { std::cerr<<"OpenGL 3.2 unavailable\n"; return 77; }
    std::cout<<"GL: "<<gl->glGetString(GL_RENDERER)<<" / "<<gl->glGetString(GL_VERSION)<<'\n';
    QOpenGLShaderProgram shader;
    if (!shader.addShaderFromSourceCode(QOpenGLShader::Vertex,mpr_gpu::vertex_shader) ||
        !shader.addShaderFromSourceCode(QOpenGLShader::Fragment,mpr_gpu::projection_shader) || !shader.link()) {
        std::cerr<<shader.log().toStdString()<<'\n'; return 1;
    }
    auto image=Image::New(); Image::IndexType start; start[0]=3; start[1]=7; start[2]=11;
    Image::SizeType size; size.Fill(9); Image::RegionType region; region.SetIndex(start); region.SetSize(size);
    image->SetRegions(region); Image::SpacingType spacing; spacing[0]=1; spacing[1]=2; spacing[2]=3; image->SetSpacing(spacing);
    Image::PointType origin; origin[0]=37.25; origin[1]=-42.5; origin[2]=11.75; image->SetOrigin(origin);
    Image::DirectionType dir; dir.SetIdentity(); dir[0][0]=0; dir[1][0]=1; dir[0][1]=-1; dir[1][1]=0; image->SetDirection(dir);
    image->Allocate(); std::vector<float> data(9*9*9);
    for (int z=0;z<9;++z) for (int y=0;y<9;++y) for (int x=0;x<9;++x) {
        short value=static_cast<short>(-1000 + x*211 + y*103 + z*127);
        Image::IndexType index; index[0]=start[0]+x; index[1]=start[1]+y; index[2]=start[2]+z;
        image->SetPixel(index,value); data[(z*9+y)*9+x]=value;
    }
    GLuint tex=0,vao=0; gl->glGenTextures(1,&tex); gl->glBindTexture(GL_TEXTURE_3D,tex);
    gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); gl->glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    gl->glTexImage3D(GL_TEXTURE_3D,0,GL_R32F,9,9,9,0,GL_RED,GL_FLOAT,data.data());
    if(gl->glGetError()!=GL_NO_ERROR) { std::cerr<<"texture allocation failed\n"; return 1; }
    QOpenGLFramebufferObjectFormat fbformat; fbformat.setInternalTextureFormat(GL_RG32F);
    QOpenGLFramebufferObject fbo(5,5,fbformat);
    if (!fbo.isValid() || !fbo.bind()) { std::cerr<<"floating framebuffer unavailable\n"; return 1; }
    gl->glGenVertexArrays(1,&vao); gl->glBindVertexArray(vao); gl->glViewport(0,0,5,5); gl->glDisable(GL_BLEND);
    Image::IndexType mid; for(int a=0;a<3;++a) mid[a]=start[a]+4;
    Image::PointType p; image->TransformIndexToPhysicalPoint(mid,p); mpr_geometry::Vector3 center{p[0],p[1],p[2]};
    const double q=std::sqrt(0.5);
    struct PlaneCase { const char *name; mpr_geometry::Vector3 u,v,offset; };
    // Volume axes are e0=(0,1,0), e1=(-1,0,0), e2=(0,0,1).
    // Boundary cases extend outside the source volume and exercise the mask.
    const PlaneCase planes[] = {
        {"axial", {0,1,0}, {-1,0,0}, {0,0,0}},
        {"coronal", {0,1,0}, {0,0,1}, {0,0,0}},
        {"sagittal", {-1,0,0}, {0,0,1}, {0,0,0}},
        {"oblique", {0,1,0}, {-q,0,q}, {0,0,0}},
        {"oblique edge", {0,1,0}, {-q,0,q}, {-7.25,0,0}},
        {"lower half voxel", {0,1,0}, {-1,0,0}, {0,0,-13.5}},
        {"upper half voxel", {0,1,0}, {-1,0,0}, {0,0,13.5}},
        {"outside", {0,1,0}, {-1,0,0}, {0,0,50}}
    };
    auto uniform3=[&](const char *name,mpr_geometry::Vector3 a) { shader.setUniformValue(name,QVector3D(float(a.x),float(a.y),float(a.z))); };
    double maxShaderError=0.0, maxBackendError=0.0;
    // A 2-unit bound is specific to this finite CT-like phantom and the tested
    // hardware interpolation; it is not a general clinical acceptance threshold.
    for (const auto &plane : planes) for (double thickness : {0.0,5.0,10.0,20.0}) for(int mode : {1,2,3}) {
        const auto u=plane.u, v=plane.v;
        const auto planeCenter=mpr_geometry::add(center,plane.offset);
        mpr_gpu::SamplingGeometry geom;
        if (!mpr_gpu::sampling_geometry(image.GetPointer(),planeCenter,u,v,5,5,1.0,1.0,thickness,geom)) return 1;
        shader.bind(); shader.setUniformValue("volume",0); shader.setUniformValue("volume_size",QVector3D(9,9,9));
        uniform3("first_index",geom.first); uniform3("index_du",geom.du); uniform3("index_dv",geom.dv); uniform3("normal_per_mm",geom.normal_per_mm);
        shader.setUniformValue("sample_step_mm",float(geom.sample_step)); shader.setUniformValue("half_samples",geom.half_samples); shader.setUniformValue("mode",mode);
        gl->glDrawArrays(GL_TRIANGLES,0,3); std::vector<float> pixels(50); gl->glReadPixels(0,0,5,5,GL_RG,GL_FLOAT,pixels.data());
        if (gl->glGetError()!=GL_NO_ERROR) { std::cerr<<"render failed\n"; return 1; }
        auto cpu=mpr_slab::render_itk(image.GetPointer(),planeCenter,u,v,5,5,1,1,thickness,mode==1?mpr_slab::Mode::Maximum:(mode==2?mpr_slab::Mode::Minimum:mpr_slab::Mode::Mean));
        if(cpu.pixels.IsNull()) return 1;
        for (int y=0;y<5;++y) for (int x=0;x<5;++x) {
            itk::Index<2> i; i[0]=x; i[1]=y; int j=2*(y*5+x);
            if (pixels[j+1]>0.5f && cpu.valid->GetPixel(i)) maxShaderError=(std::max)(maxShaderError, std::abs(double(pixels[j])-cpu.pixels->GetPixel(i)));
            if ((pixels[j+1]>0.5f)!=(cpu.valid->GetPixel(i)!=0) || (pixels[j+1]>0.5f && std::abs(pixels[j]-cpu.pixels->GetPixel(i))>2.0)) {
                std::cerr<<"GPU/CPU mismatch plane="<<plane.name<<" thickness="<<thickness<<" mode="<<mode<<" pixel="<<x<<","<<y<<" gpu="<<pixels[j]<<" cpu="<<cpu.pixels->GetPixel(i)<<'\n'; return 1;
            }
        }
    }
    // Exercise the same renderer used by the viewer, with consecutive views
    // sharing the singleton volume, as well as cache invalidation and fallback.
    auto &renderer=MprGpuRenderer::instance();
    for (const auto &plane : planes) for (double thickness : {0.0,5.0,10.0,20.0}) for (int mode : {0,1,2,3}) {
        const auto planeCenter=mpr_geometry::add(center,plane.offset);
        auto gpu=renderer.render(image.GetPointer(),planeCenter,plane.u,plane.v,5,5,1,1,thickness,mode);
        const auto cpuMode=mode==1?mpr_slab::Mode::Maximum:(mode==2?mpr_slab::Mode::Minimum:mpr_slab::Mode::Mean);
        auto cpu=mpr_slab::render_itk(image.GetPointer(),planeCenter,plane.u,plane.v,5,5,1,1,thickness,cpuMode);
        if (gpu.pixels.IsNull() || gpu.valid.IsNull() || cpu.pixels.IsNull()) { std::cerr<<"backend failed\n"; return 1; }
        for (int y=0;y<5;++y) for (int x=0;x<5;++x) {
            itk::Index<2> i; i[0]=x; i[1]=y;
            if (gpu.valid->GetPixel(i) && cpu.valid->GetPixel(i)) maxBackendError=(std::max)(maxBackendError, std::abs(gpu.pixels->GetPixel(i)-cpu.pixels->GetPixel(i)));
            if (gpu.valid->GetPixel(i)!=cpu.valid->GetPixel(i) ||
                (cpu.valid->GetPixel(i) && std::abs(gpu.pixels->GetPixel(i)-cpu.pixels->GetPixel(i))>2.0)) {
                std::cerr<<"backend mismatch plane="<<plane.name<<" thickness="<<thickness<<" mode="<<mode<<" at "<<x<<","<<y<<'\n'; return 1;
            }
        }
    }
    if (!test_scalar_type<int>(renderer,"int32") || !test_scalar_type<unsigned int>(renderer,"uint32") ||
        !test_scalar_type<float>(renderer,"float32")) return 1;
    {
        using I=itk::Image<int,3>; auto volume=I::New(); I::IndexType idx; idx.Fill(0); I::SizeType size; size.Fill(1);
        I::RegionType region; region.SetIndex(idx); region.SetSize(size); volume->SetRegions(region); volume->Allocate();
        for (int value : {16777216,-16777216}) {
            volume->SetPixel(idx,value); volume->Modified();
            auto valid=renderer.render(volume.GetPointer(),{0,0,0},{1,0,0},{0,1,0},1,1,1,1,0,0);
            itk::Index<2> p; p.Fill(0);
            if(valid.pixels.IsNull() || valid.pixels->GetPixel(p)!=value) { std::cerr<<"2^24 must be exact\n"; return 1; }
        }
        for (int value : {16777217,-16777217}) {
            volume->SetPixel(idx,value); volume->Modified();
            if(renderer.render(volume.GetPointer(),{0,0,0},{1,0,0},{0,1,0},1,1,1,1,0,0).pixels.IsNotNull()) {
                std::cerr<<"out-of-range int must be rejected\n"; return 1;
            }
        }
    }
    {
        using F=itk::Image<float,3>; auto volume=F::New(); F::IndexType idx; idx.Fill(0); F::SizeType size; size.Fill(1);
        F::RegionType region; region.SetIndex(idx); region.SetSize(size); volume->SetRegions(region); volume->Allocate();
        volume->SetPixel(idx,std::numeric_limits<float>::quiet_NaN());
        if(renderer.render(volume.GetPointer(),{0,0,0},{1,0,0},{0,1,0},1,1,1,1,0,0).pixels.IsNotNull()) {
            std::cerr<<"nonfinite float must be rejected\n"; return 1;
        }
    }
    // A modified source must be re-uploaded even when its pointer is unchanged.
    image->SetPixel(mid,1234); image->Modified();
    auto changed=renderer.render(image.GetPointer(),center,planes[0].u,planes[0].v,1,1,1,1,0,0);
    itk::Index<2> first; first.Fill(0);
    if(changed.pixels.IsNull() || std::abs(changed.pixels->GetPixel(first)-1234)>2.0) {
        std::cerr<<"backend stale volume\n"; return 1;
    }
    renderer.releaseVolume();
    auto reloaded=renderer.render(image.GetPointer(),center,planes[0].u,planes[0].v,1,1,1,1,0,0);
    if(reloaded.pixels.IsNull() || std::abs(reloaded.pixels->GetPixel(first)-1234)>2.0) {
        std::cerr<<"backend could not reload released volume\n"; return 1;
    }
    // The renderer must restore an already-current context after its work.
    if(QOpenGLContext::currentContext()!=&context) { std::cerr<<"renderer did not restore context\n"; return 1; }
    auto excessive=renderer.render(image.GetPointer(),center,planes[0].u,planes[0].v,512,512,1,1,120,1);
    if(excessive.pixels.IsNotNull()) { std::cerr<<"backend must reject excessive work\n"; return 1; }
    auto invalid=renderer.render(image.GetPointer(),center,planes[0].u,planes[0].v,0,1,1,1,0,0);
    if(invalid.pixels.IsNotNull()) { std::cerr<<"backend must reject invalid size\n"; return 1; }
    gl->glDeleteVertexArrays(1,&vao); gl->glDeleteTextures(1,&tex);
    std::cout<<"GPU/CPU projections and renderer passed; max error shader="<<maxShaderError<<" backend="<<maxBackendError<<'\n'; return 0;
}

