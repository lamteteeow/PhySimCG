#include <scenes.hpp>

#include <vislab/graphics/bmp_writer.hpp>
#include <vislab/graphics/image.hpp>
#include <vislab/graphics/path_radiance_integrator.hpp>

int main()
{
    using namespace vislab;

    // image buffer to render into.
    auto image = std::make_shared<Image3d>();
    image->setResolution(128, 128);
    image->setZero();

    // create a scene
    auto scene = pbr::Scenes::create_box_scene(image->getResolution());

    // create a light transport integrator and render the image.
    auto integrator = std::make_unique<PathRadianceIntegrator>();
    integrator->setSamplesPerPixel(512);
    integrator->maxDepth      = 4;
    integrator->rouletteDepth = 2;
    integrator->render(scene.get(), image.get());

    // write image to file
    auto writer = std::make_unique<BmpWriter>();
    writer->inputImage.setData(image);
    writer->paramPath.setValue("path.bmp");
    writer->update();

    return 0;
}
