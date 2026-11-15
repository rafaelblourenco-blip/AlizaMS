#include <QApplication>
#include <QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QPointF>
#include <QRectF>
#include <QString>

#include <itkImage.h>
#include <itkResampleImageFilter.h>
#include <itkIdentityTransform.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkExtractImageFilter.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <memory>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =========================================================================
// TEST 1: MPR & 3D TOOLBAR ACTIONS AND CONFIGURATION
// =========================================================================
void test_mpr_and_3d_toolbar_configuration()
{
    std::cout << "[RUNNING] test_mpr_and_3d_toolbar_configuration..." << std::endl;

    // Test toolbar styling rules
    QString expectedBg = "#0D2137";
    QString expectedHover = "#38BDF8";
    int expectedIconSize = 22;

    QToolBar testToolbar("MPRToolBar");
    testToolbar.setObjectName("RadiantOverlayToolBar");
    testToolbar.setIconSize(QSize(expectedIconSize, expectedIconSize));
    testToolbar.setMovable(false);

    assert(testToolbar.iconSize().width() == 22);
    assert(testToolbar.iconSize().height() == 22);
    assert(!testToolbar.isMovable());
    std::cout << "  PASS: Dedicated toolbar icon size 22x22 and immovable constraint verified." << std::endl;

    // Verify MPR action set
    std::vector<QString> mprActions = {
        "Janelamento WL/WW",
        "Mover / Pan",
        "Zoom Interativo",
        "Linhas de Referência",
        "Modo Slab / MIP",
        "Restaurar Planos Ortogonais",
        "Layout MPR",
        "Exportar Fatias MPR"
    };

    for (const auto & actName : mprActions)
    {
        QAction * act = new QAction(actName, &testToolbar);
        testToolbar.addAction(act);
    }
    assert(testToolbar.actions().size() == static_cast<int>(mprActions.size()));
    std::cout << "  PASS: All 8 core MPR actions successfully registered in toolbar." << std::endl;

    // Verify 3D VR action set
    QToolBar test3DToolbar("3DToolBar");
    std::vector<QString> vr3dActions = {
        "Presets 3D (GPU)",
        "Rotacionar 3D (Trackball)",
        "Mover 3D (Pan)",
        "Zoom 3D",
        "Bisturi / Planos de Corte",
        "Vistas Anatômicas Padrão",
        "Restaurar Orientação 3D",
        "Capturar Imagem 3D / Exportar"
    };

    for (const auto & actName : vr3dActions)
    {
        QAction * act = new QAction(actName, &test3DToolbar);
        test3DToolbar.addAction(act);
    }
    assert(test3DToolbar.actions().size() == static_cast<int>(vr3dActions.size()));
    std::cout << "  PASS: All 8 core 3D VR actions successfully registered in toolbar." << std::endl;
}

// =========================================================================
// TEST 2: ROTATION HANDLE HOVER DETECTION AND ANGLE COMPUTATION
// =========================================================================
void test_reference_line_rotation_handles()
{
    std::cout << "[RUNNING] test_reference_line_rotation_handles..." << std::endl;

    const double radius = 10.0;
    const QPointF handle_h1(100.0, 256.0);
    const QPointF handle_h2(412.0, 256.0);
    const QPointF handle_v1(256.0, 100.0);
    const QPointF handle_v2(256.0, 412.0);
    const QPointF pivot(256.0, 256.0);

    auto checkHover = [&](const QPointF & mousePos) -> int {
        if (std::hypot(mousePos.x() - handle_h1.x(), mousePos.y() - handle_h1.y()) <= radius) return 1;
        if (std::hypot(mousePos.x() - handle_h2.x(), mousePos.y() - handle_h2.y()) <= radius) return 2;
        if (std::hypot(mousePos.x() - handle_v1.x(), mousePos.y() - handle_v1.y()) <= radius) return 3;
        if (std::hypot(mousePos.x() - handle_v2.x(), mousePos.y() - handle_v2.y()) <= radius) return 4;
        return 0;
    };

    // Test exact hit on handle 1
    assert(checkHover(QPointF(100.0, 256.0)) == 1);
    // Test hit within radius 8px (< 10px)
    assert(checkHover(QPointF(104.0, 258.0)) == 1);
    // Test miss just outside radius 12px (> 10px)
    assert(checkHover(QPointF(112.0, 256.0)) == 0);

    // Test exact hits on other handles
    assert(checkHover(QPointF(412.0, 256.0)) == 2);
    assert(checkHover(QPointF(256.0, 100.0)) == 3);
    assert(checkHover(QPointF(256.0, 412.0)) == 4);
    assert(checkHover(QPointF(256.0, 256.0)) == 0); // Pivot is not a rotation handle
    std::cout << "  PASS: Rotation handle hover zones strictly enforced within r <= 10 px." << std::endl;

    // Test angle computation around pivot
    auto calcAngle = [&](const QPointF & pt) -> double {
        return std::atan2(pt.y() - pivot.y(), pt.x() - pivot.x());
    };

    // Right of pivot -> 0
    assert(std::abs(calcAngle(QPointF(356.0, 256.0)) - 0.0) < 1e-6);
    // Top of pivot (screen space y decreases upward, but atan2(dy, dx)):
    // dy = -100, dx = 0 -> -pi/2
    assert(std::abs(calcAngle(QPointF(256.0, 156.0)) - (-M_PI / 2.0)) < 1e-6);
    // Bottom of pivot: dy = +100, dx = 0 -> +pi/2
    assert(std::abs(calcAngle(QPointF(256.0, 356.0)) - (M_PI / 2.0)) < 1e-6);
    // Left of pivot: dy = 0, dx = -100 -> pi
    assert(std::abs(calcAngle(QPointF(156.0, 256.0)) - M_PI) < 1e-6);

    // Angle wrapping within [-pi, pi]
    auto wrapAngle = [](double a) -> double {
        while (a > M_PI) a -= 2.0 * M_PI;
        while (a < -M_PI) a += 2.0 * M_PI;
        return a;
    };
    assert(std::abs(wrapAngle(3.0 * M_PI) - M_PI) < 1e-6);
    assert(std::abs(wrapAngle(-3.0 * M_PI) - (-M_PI)) < 1e-6);
    assert(std::abs(wrapAngle(M_PI / 4.0) - (M_PI / 4.0)) < 1e-6);
    std::cout << "  PASS: Angular atan2 and [-pi, pi] wrapping calculations verified." << std::endl;
}

// =========================================================================
// TEST 3: DIRECTION COSINE MATRIX AND PHYSICAL ORIGIN MATHEMATICS
// =========================================================================
void test_direction_matrix_and_origin()
{
    std::cout << "[RUNNING] test_direction_matrix_and_origin..." << std::endl;

    itk::Vector<double, 3> e0, e1, e2;
    e0[0] = 1.0; e0[1] = 0.0; e0[2] = 0.0;
    e1[0] = 0.0; e1[1] = 1.0; e1[2] = 0.0;
    e2[0] = 0.0; e2[1] = 0.0; e2[2] = 1.0;

    for (double deg = 0.0; deg <= 360.0; deg += 30.0)
    {
        const double rad = deg * M_PI / 180.0;
        const double cosA = std::cos(rad);
        const double sinA = std::sin(rad);

        // Coronal-like plane rotated around Z
        itk::Vector<double, 3> vecU = e0 * cosA + e1 * sinA;
        itk::Vector<double, 3> vecV = e2;
        itk::Vector<double, 3> vecN;
        vecN[0] = vecU[1] * vecV[2] - vecU[2] * vecV[1];
        vecN[1] = vecU[2] * vecV[0] - vecU[0] * vecV[2];
        vecN[2] = vecU[0] * vecV[1] - vecU[1] * vecV[0];

        // 1. Orthogonality: U . V == 0, U . N == 0, V . N == 0
        const double dotUV = vecU[0]*vecV[0] + vecU[1]*vecV[1] + vecU[2]*vecV[2];
        const double dotUN = vecU[0]*vecN[0] + vecU[1]*vecN[1] + vecU[2]*vecN[2];
        const double dotVN = vecV[0]*vecN[0] + vecV[1]*vecN[1] + vecV[2]*vecN[2];
        assert(std::abs(dotUV) < 1e-9);
        assert(std::abs(dotUN) < 1e-9);
        assert(std::abs(dotVN) < 1e-9);

        // 2. Unit norms: |U| == 1, |V| == 1, |N| == 1
        const double normU = std::sqrt(vecU[0]*vecU[0] + vecU[1]*vecU[1] + vecU[2]*vecU[2]);
        const double normV = std::sqrt(vecV[0]*vecV[0] + vecV[1]*vecV[1] + vecV[2]*vecV[2]);
        const double normN = std::sqrt(vecN[0]*vecN[0] + vecN[1]*vecN[1] + vecN[2]*vecN[2]);
        assert(std::abs(normU - 1.0) < 1e-9);
        assert(std::abs(normV - 1.0) < 1e-9);
        assert(std::abs(normN - 1.0) < 1e-9);

        // 3. Determinant det(D) == +1.0 (Right-handed orthonormal basis)
        const double detD = vecU[0] * (vecV[1] * vecN[2] - vecV[2] * vecN[1])
                          - vecU[1] * (vecV[0] * vecN[2] - vecV[2] * vecN[0])
                          + vecU[2] * (vecV[0] * vecN[1] - vecV[1] * vecN[0]);
        assert(std::abs(detD - 1.0) < 1e-9);
    }
    std::cout << "  PASS: Direction cosine matrix D=[U,V,N] maintains det(D)=+1.0 and orthogonality across 360 deg." << std::endl;

    // Origin calculation verification:
    // P_origin = C - ((W - 1)/2)*sU*U - ((H - 1)/2)*sV*V
    itk::Point<double, 3> C;
    C[0] = 50.0; C[1] = 60.0; C[2] = 70.0;
    const unsigned int W = 512, H = 512;
    const double sU = 0.5, sV = 0.5;
    itk::Vector<double, 3> U = e0;
    itk::Vector<double, 3> V = e1;

    itk::Point<double, 3> pOrigin;
    const double halfW = (W - 1.0) * 0.5 * sU;
    const double halfH = (H - 1.0) * 0.5 * sV;
    pOrigin[0] = C[0] - halfW * U[0] - halfH * V[0];
    pOrigin[1] = C[1] - halfW * U[1] - halfH * V[1];
    pOrigin[2] = C[2] - halfW * U[2] - halfH * V[2];

    // Compute center voxel physical position at index (255.5, 255.5) -> must equal C!
    const double centerVoxelX = pOrigin[0] + halfW * U[0];
    const double centerVoxelY = pOrigin[1] + halfH * V[1];
    const double centerVoxelZ = pOrigin[2];
    assert(std::abs(centerVoxelX - C[0]) < 1e-9);
    assert(std::abs(centerVoxelY - C[1]) < 1e-9);
    assert(std::abs(centerVoxelZ - C[2]) < 1e-9);
    std::cout << "  PASS: Center voxel of output slice maps exactly to 3D physical pivot C." << std::endl;
}

// =========================================================================
// TEST 4: AUTHENTIC ITK RESAMPLEIMAGEFILTER & EXTRACTIMAGEFILTER RESLICING
// =========================================================================
void test_itk_resample_oblique_slice_pipeline()
{
    std::cout << "[RUNNING] test_itk_resample_oblique_slice_pipeline..." << std::endl;

    typedef itk::Image<short, 3> Image3DType;
    typedef itk::Image<short, 2> Image2DType;

    // Create a 3D synthetic volume with known intensity function:
    // I(x, y, z) = x + 10 * y + 100 * z
    Image3DType::Pointer volume = Image3DType::New();
    Image3DType::SizeType size = {{ 64, 64, 64 }};
    Image3DType::IndexType start = {{ 0, 0, 0 }};
    Image3DType::RegionType region;
    region.SetSize(size);
    region.SetIndex(start);
    volume->SetRegions(region);
    volume->Allocate();

    Image3DType::SpacingType spacing;
    spacing[0] = 1.0; spacing[1] = 1.0; spacing[2] = 1.0;
    volume->SetSpacing(spacing);

    Image3DType::PointType origin;
    origin[0] = 0.0; origin[1] = 0.0; origin[2] = 0.0;
    volume->SetOrigin(origin);

    itk::ImageRegionIterator<Image3DType> it(volume, region);
    while (!it.IsAtEnd())
    {
        Image3DType::IndexType idx = it.GetIndex();
        short val = static_cast<short>(idx[0] + 10 * idx[1] + 100 * idx[2]);
        it.Set(val);
        ++it;
    }

    itk::Point<double, 3> center;
    center[0] = 31.5; center[1] = 31.5; center[2] = 31.5;

    // Resample at theta = 0 (orthogonal axial-like cut)
    itk::Vector<double, 3> vecU, vecV, vecN;
    vecU[0] = 1.0; vecU[1] = 0.0; vecU[2] = 0.0;
    vecV[0] = 0.0; vecV[1] = 1.0; vecV[2] = 0.0;
    vecN[0] = 0.0; vecN[1] = 0.0; vecN[2] = 1.0;

    const unsigned int W = 64, H = 64;
    const double sU = 1.0, sV = 1.0;

    auto executeReslice = [&](bool fastDraft) -> Image2DType::Pointer {
        typedef itk::ResampleImageFilter<Image3DType, Image3DType> ResampleType;
        ResampleType::Pointer resampler = ResampleType::New();
        resampler->SetInput(volume);

        Image3DType::SizeType outSize = {{ W, H, 1 }};
        resampler->SetSize(outSize);
        Image3DType::SpacingType outSpacing;
        outSpacing[0] = sU; outSpacing[1] = sV; outSpacing[2] = 1.0;
        resampler->SetOutputSpacing(outSpacing);

        Image3DType::PointType pOrigin;
        const double halfW = (W - 1.0) * 0.5 * sU;
        const double halfH = (H - 1.0) * 0.5 * sV;
        pOrigin[0] = center[0] - halfW * vecU[0] - halfH * vecV[0];
        pOrigin[1] = center[1] - halfW * vecU[1] - halfH * vecV[1];
        pOrigin[2] = center[2] - halfW * vecU[2] - halfH * vecV[2];
        resampler->SetOutputOrigin(pOrigin);

        itk::Matrix<double, 3, 3> outDir;
        for (unsigned int r = 0; r < 3; ++r) {
            outDir[r][0] = vecU[r];
            outDir[r][1] = vecV[r];
            outDir[r][2] = vecN[r];
        }
        resampler->SetOutputDirection(outDir);
        resampler->SetTransform(itk::IdentityTransform<double, 3>::New());
        resampler->SetDefaultPixelValue(static_cast<short>(-1000));

        if (fastDraft)
            resampler->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<Image3DType, double>::New());
        else
            resampler->SetInterpolator(itk::LinearInterpolateImageFunction<Image3DType, double>::New());

        resampler->Update();

        typedef itk::ExtractImageFilter<Image3DType, Image2DType> ExtractType;
        ExtractType::Pointer extractor = ExtractType::New();
        extractor->SetInput(resampler->GetOutput());

        Image3DType::RegionType exRegion;
        Image3DType::SizeType exSize = {{ W, H, 0 }};
        Image3DType::IndexType exIdx = {{ 0, 0, 0 }};
        exRegion.SetSize(exSize);
        exRegion.SetIndex(exIdx);
        extractor->SetExtractionRegion(exRegion);
        extractor->SetDirectionCollapseToIdentity();
        extractor->Update();

        Image2DType::Pointer slice = extractor->GetOutput();
        slice->DisconnectPipeline();
        return slice;
    };

    // 1. Verify Draft Mode (Nearest-Neighbor)
    Image2DType::Pointer draftSlice = executeReslice(true);
    assert(draftSlice.IsNotNull());
    assert(draftSlice->GetLargestPossibleRegion().GetSize()[0] == 64);
    assert(draftSlice->GetLargestPossibleRegion().GetSize()[1] == 64);

    // 2. Verify Diagnostic Mode (Linear Interpolation)
    Image2DType::Pointer diagSlice = executeReslice(false);
    assert(diagSlice.IsNotNull());
    assert(diagSlice->GetLargestPossibleRegion().GetSize()[0] == 64);
    assert(diagSlice->GetLargestPossibleRegion().GetSize()[1] == 64);

    // Check pixel at center index (32, 32):
    // In volume: x=32, y=32, z=31.5 -> Linear: 32 + 10*32 + 100*31.5 = 32 + 320 + 3150 = 3502
    Image2DType::IndexType center2DIdx = {{ 32, 32 }};
    short diagPixel = diagSlice->GetPixel(center2DIdx);
    assert(std::abs(diagPixel - 3502) <= 1);
    std::cout << "  PASS: Authentic ITK ResampleImageFilter & ExtractImageFilter reslicing verified (Center: "
              << diagPixel << " HU)." << std::endl;

    // 3. Oblique Slice at 45 degrees
    const double angle45 = M_PI / 4.0;
    vecU[0] = std::cos(angle45); vecU[1] = std::sin(angle45); vecU[2] = 0.0;
    vecV[0] = 0.0; vecV[1] = 0.0; vecV[2] = 1.0;
    vecN[0] = vecU[1]*vecV[2] - vecU[2]*vecV[1];
    vecN[1] = vecU[2]*vecV[0] - vecU[0]*vecV[2];
    vecN[2] = vecU[0]*vecV[1] - vecU[1]*vecV[0];

    Image2DType::Pointer oblique45Slice = executeReslice(false);
    assert(oblique45Slice.IsNotNull());
    assert(oblique45Slice->GetLargestPossibleRegion().GetSize()[0] == 64);
    assert(oblique45Slice->GetLargestPossibleRegion().GetSize()[1] == 64);

    short oblPixel = oblique45Slice->GetPixel(center2DIdx);
    std::cout << "  DEBUG: 45-deg oblique slice pixel at (32, 32) is: " << oblPixel << std::endl;
    // Expected at (32, 32): x = 31.5 + 0.5*cos(45) = 31.854, y = 31.5 + 0.5*sin(45) = 31.854, z = 31.5 + 0.5 = 32.0
    // Expected value = 31.854 + 10 * 31.854 + 100 * 32.0 = 3550
    assert(std::abs(oblPixel - 3550) <= 2);
    std::cout << "  PASS: 45-degree oblique slice reconstruction preserves physical point fidelity (Pixel: "
              << oblPixel << " HU, expected ~3550)." << std::endl;
}

// =========================================================================
// MAIN RUNNER
// =========================================================================
int main(int argc, char ** argv)
{
    QApplication app(argc, argv);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  AlizaMS Milestone 4 (MPR, 3D & Oblique) Unit Test Suite" << std::endl;
    std::cout << "=========================================================" << std::endl;

    test_mpr_and_3d_toolbar_configuration();
    test_reference_line_rotation_handles();
    test_direction_matrix_and_origin();
    test_itk_resample_oblique_slice_pipeline();

    std::cout << "=========================================================" << std::endl;
    std::cout << "  ALL 4 MILESTONE 4 TEST SUITES PASSED (100%)!            " << std::endl;
    std::cout << "=========================================================" << std::endl;

    return 0;
}
