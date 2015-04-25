#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

#define SCENE_SIZE 512
#define BLUR_SIZE 128

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderBlurApp : public AppNative {
  public:
    void prepareSettings( Settings *settings );
	void setup();
	void update();
	void draw();
    
    void render();
    
    void keyDown( KeyEvent event );
    
    void drawStrokedRect( const Rectf &rect );

    ci::Font		mFont;
    
    gl::Fbo			mFboScene;
    gl::Fbo			mFboBlur1;
    gl::Fbo			mFboBlur2;
    
    gl::GlslProg	mShaderBlur;
    
    CameraPersp		mCamera;
};

void CinderBlurApp::prepareSettings(cinder::app::AppBasic::Settings *settings)
{
    settings->setWindowSize(1606, 400);
    settings->setTitle("Gaussian blur demo");
}

void CinderBlurApp::setup()
{
    mFont = Font("Arial", 24.0f);
    
    gl::Fbo::Format fmt;
    fmt.setSamples(8);
    fmt.setCoverageSamples(8);
    
    // setup our scene Fbo
    mFboScene = gl::Fbo(SCENE_SIZE, SCENE_SIZE, fmt);
    
    // setup our blur Fbo's, smaller ones will generate a bigger blur
    mFboBlur1 = gl::Fbo(BLUR_SIZE, BLUR_SIZE);
    mFboBlur2 = gl::Fbo(BLUR_SIZE, BLUR_SIZE);
    
    // if we don't want to view our FBO's upside-down, we'll have to flip them
    mFboScene.getTexture(0).setFlipped();
    mFboBlur1.getTexture(0).setFlipped();
    mFboBlur2.getTexture(0).setFlipped();
    
    // load and compile the shaders
    try {
        mShaderBlur = gl::GlslProg( loadAsset("blur.vert"), loadAsset("blur.frag"));
    }
    catch( const std::exception &e ) {
        console() << e.what() << endl;
        quit();
    }
    
    mCamera.setEyePoint( Vec3f(0.0f, 8.0f, 25.0f) );
    mCamera.setCenterOfInterestPoint( Vec3f(0.0f, -1.0f, 0.0f) );
    mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
}

void CinderBlurApp::update()
{
//    mTransform.setToIdentity();
//    mTransform.rotate( Vec3f::xAxis(), (float) getElapsedSeconds() * 0.4f );
//    mTransform.rotate( Vec3f::yAxis(), (float) getElapsedSeconds() * 0.2f );
    
}

void CinderBlurApp::draw()
{
    // clear our window
    gl::clear( Color::black() );
    
    // store our viewport, so we can restore it later
    Area viewport = gl::getViewport();
    
    gl::setViewport( mFboScene.getBounds() );
    
    mFboScene.bindFramebuffer();
        gl::pushMatrices();
            gl::setMatricesWindow(SCENE_SIZE, SCENE_SIZE, false);
            gl::clear( Color::black() );
            render();
        gl::popMatrices();
    mFboScene.unbindFramebuffer();
    
    // bind the blur shader
    mShaderBlur.bind();
    mShaderBlur.uniform("tex0", 0); // use texture unit 0
    
    // tell the shader to blur horizontally and the size of 1 pixel
    mShaderBlur.uniform("sample_offset", Vec2f(1.0f/mFboBlur1.getWidth(), 0.0f));
    mShaderBlur.uniform("attenuation", 2.5f);
    
    // copy a horizontally blurred version of our scene into the first blur Fbo
    gl::setViewport( mFboBlur1.getBounds() );
    mFboBlur1.bindFramebuffer();
        mFboScene.bindTexture(0);
            gl::pushMatrices();
                gl::setMatricesWindow(BLUR_SIZE, BLUR_SIZE, false);
                gl::clear( Color::black() );
                gl::drawSolidRect( mFboBlur1.getBounds() );
            gl::popMatrices();
        mFboScene.unbindTexture();
    mFboBlur1.unbindFramebuffer();
    
    // tell the shader to blur vertically and the size of 1 pixel
    mShaderBlur.uniform("sample_offset", Vec2f(0.0f, 1.0f/mFboBlur2.getHeight()));
    mShaderBlur.uniform("attenuation", 2.5f);
    
    // copy a vertically blurred version of our blurred scene into the second blur Fbo
    gl::setViewport( mFboBlur2.getBounds() );
    mFboBlur2.bindFramebuffer();
        mFboBlur1.bindTexture(0);
            gl::pushMatrices();
                gl::setMatricesWindow(BLUR_SIZE, BLUR_SIZE, false);
                gl::clear( Color::black() );
                gl::drawSolidRect( mFboBlur2.getBounds() );
            gl::popMatrices();
        mFboBlur1.unbindTexture();
    mFboBlur2.unbindFramebuffer();
    
    // unbind the shader
    mShaderBlur.unbind();
    
    // render scene into mFboScene using color texture
    gl::setViewport( mFboScene.getBounds() );
    mFboScene.bindFramebuffer();
        gl::pushMatrices();
            gl::setMatricesWindow(SCENE_SIZE, SCENE_SIZE, false);
            gl::clear( Color::black() );
            render();
        gl::popMatrices();
    mFboScene.unbindFramebuffer();
    
    // restore the viewport
    gl::setViewport( viewport );
    
    // draw the 3 Fbo's
    gl::color( Color::white() );
    gl::draw( mFboScene.getTexture(), Rectf(0, 0, 400, 400) );
    drawStrokedRect( Rectf(0, 0, 400, 400) );
    
    gl::draw( mFboBlur1.getTexture(), Rectf(402, 0, 402 + 400, 400) );
    drawStrokedRect( Rectf(402, 0, 402 + 400, 400) );
    
    gl::draw( mFboBlur2.getTexture(), Rectf(804, 0, 804 + 400, 400) );
    drawStrokedRect( Rectf(804, 0, 804 + 400, 400) );
    
    // draw our scene with the blurred version added as a blend
    gl::color( Color::white() );
    gl::draw( mFboScene.getTexture(), Rectf(1206, 0, 1206 + 400, 400) );
    
    gl::enableAdditiveBlending();
    gl::draw( mFboBlur2.getTexture(), Rectf(1206, 0, 1206 + 400, 400) );
    gl::disableAlphaBlending();
    drawStrokedRect( Rectf(1206, 0, 1206 + 400, 400) );
}

void CinderBlurApp::keyDown(cinder::app::KeyEvent event)
{

}

void CinderBlurApp::render()
{
    // get the current viewport
    Area viewport = gl::getViewport();
    
    // adjust the aspect ratio of the camera
    mCamera.setAspectRatio( viewport.getWidth() / (float) viewport.getHeight() );
    
    // render our scene (see the Picking3D sample for more info)
    gl::pushMatrices();
    
    gl::color( Color::white() );
    gl::drawSolidCircle(Vec2f(200.0f, 200.0f), 50.0f);
    
    gl::popMatrices();
}

void CinderBlurApp::drawStrokedRect( const Rectf &rect )
{
    // we don't want any texture on our lines
    glDisable(GL_TEXTURE_2D);
    
    gl::drawLine(rect.getUpperLeft(), rect.getUpperRight());
    gl::drawLine(rect.getUpperRight(), rect.getLowerRight());
    gl::drawLine(rect.getLowerRight(), rect.getLowerLeft());
    gl::drawLine(rect.getLowerLeft(), rect.getUpperLeft());
}

CINDER_APP_NATIVE( CinderBlurApp, RendererGl )
