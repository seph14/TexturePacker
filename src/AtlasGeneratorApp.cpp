//seph li
//http://solid-jellyfish.com
//2014-01-21
//an atlas generator based on Paul.Houx's bin packaging code
//(https://github.com/cinder/Cinder/pull/362)

//known issue:
//1. load a folder of textures in multi bin mode after clearing all textures will cause a rendering issue

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/Xml.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Timer.h"

#include "BinPacker.h"
#include "Resources.h"
#include "ciUI.h"

#include <unordered_map>

using namespace ci;
using namespace ci::app;
using namespace std;

class AtlasGeneratorApp : public AppNative {
public:
    enum Mode { SINGLE, MULTI };
    
    void setupUI();
    void guiEvent(ciUIEvent *event);
    
    void resizePackage( int w );
    void clear();
    
    bool addFolder();
    bool addTexture();
    
    void exportPackage();
    void doPack();
    
    void prepareSettings(Settings *settings);
	void setup();
	void update();
	void draw();
    void shutdown();
    
    //for hacking to solve the ciUI related bug
    Timer                                       hackTimer;
    
    ciUICanvas                                  *gui;
    BinPacker                                   mBinPackerSingle;
	MultiBinPacker                              mBinPackerMulti;
    
    std::vector<string>                         mTexName;
    std::vector<gl::TextureRef>                 mTexSrc;
    
    std::vector<Area>                           mUnpackedArea;
	std::vector<BinnedArea>                     mPackedArea;
    
    bool                                        mFlipped;
    
    Mode                                        mMode;

};

void AtlasGeneratorApp::prepareSettings(Settings *settings){
	settings->setWindowSize(640, 512);
    settings->setResizable(false);
}

void AtlasGeneratorApp::setup(){
    
    glLineWidth(1.f);
    gl::enableVerticalSync();
    gl::enableAlphaBlending();
    
    mFlipped = false;
    mMode    = SINGLE;
    
	mBinPackerSingle.setSize( 128, 128 );
	mBinPackerMulti.setSize( 128, 128 );
    
	mTexSrc.clear();
	mTexName.clear();
    mUnpackedArea.clear();
	mPackedArea.clear();
	
    //hacking the timer to solve the ciUI related bug
    hackTimer.start();
    
    setupUI();
	getWindow()->setTitle( "AtlasGenerator | Single Mode | " + toString(mUnpackedArea.size()) );
    
}

void AtlasGeneratorApp::setupUI(){
    gui = new ciUICanvas(0,0,128, 512);
	
    vector<string> names; names.push_back("Single"); names.push_back("Multi");
    gui->addWidgetDown(new ciUIRadio(8, 8, "Packing Mode", names, CI_UI_ORIENTATION_HORIZONTAL));
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUITextInput(108, "size", "128", CI_UI_FONT_SMALL));
    gui->addWidgetDown(new ciUILabelButton(108, false, "Set Bin Size", CI_UI_FONT_SMALL));
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUILabelButton(108, false, "Add Texture", CI_UI_FONT_SMALL));
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUILabelButton(108, false, "Add Folder", CI_UI_FONT_SMALL));
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUIToggle(12, 12, false, "Flip Texture"));
    gui->addWidgetDown(new ciUILabelButton(108, false, "Export", CI_UI_FONT_SMALL));
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUILabelButton(108, false, "Clear", CI_UI_FONT_SMALL));
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUILabel("action1", "no last action",   CI_UI_FONT_SMALL)  );
    gui->addWidgetDown(new ciUILabel("action2", "",                 CI_UI_FONT_SMALL)  );
    gui->addWidgetDown(new ciUILabel("action3", "",                 CI_UI_FONT_SMALL)  );
    gui->addWidgetDown(new ciUILabel("action4", "",                 CI_UI_FONT_SMALL)  );
    gui->addWidgetDown(new ciUILabel("action5", "",                 CI_UI_FONT_SMALL)  );
    gui->addWidgetDown(new ciUILabel("action6", "",                 CI_UI_FONT_SMALL)  );
    
    gui->addWidgetDown(new ciUISpacer(108, 2), CI_UI_ALIGN_LEFT);
    gui->addWidgetDown(new ciUILabel(0, 912, "info", "size: 100%",  CI_UI_FONT_SMALL)  );
    
    ((ciUIRadio *)gui->getWidget("Packing Mode"))->getToggles()[0]->setValue(true);
    gui->registerUIEvents(this, &AtlasGeneratorApp::guiEvent);
}

void AtlasGeneratorApp::guiEvent(ciUIEvent *event){
    
    //handle ciUI events
	string name = event->widget->getName();
    if( name == "Single" ){
        mMode = SINGLE;
        getWindow()->setTitle( "AtlasGenerator | Single Bin | " + toString(mUnpackedArea.size()) );
        doPack();
    }else if( name == "Multi" ){
        mMode = MULTI;
        getWindow()->setTitle( "AtlasGenerator | Multi Bin | " + toString(mUnpackedArea.size()) );
        doPack();
    }else if( name == "Add Texture" ){
        //hack: this appears to be a ciUI bug that this button is called twice if i open an file window
        if( hackTimer.getSeconds() < .2f )
            return;
        if(addTexture())
            doPack();
    }else if( name == "Add Folder" ){
        //hack: this appears to be a ciUI bug that this button is called twice if i open an file window
        if( hackTimer.getSeconds() < .2f )
            return;
        if(addFolder())
            doPack();
    }else if( name == "Export" ){
        //hack: this appears to be a ciUI bug that this button is called twice if i open an file window
        if( hackTimer.getSeconds() < .2f )
            return;
        exportPackage();
    }else if( name == "Clear" )
        clear();
    else if( name == "Flip Texture" )
        mFlipped = !mFlipped;
    else if( name == "Set Bin Size" ){
        try{
            int size = fromString<int>(((ciUITextInput *)gui->getWidget("size"))->getTextString());
            float log2base = 1.f / math<float>::log(2.f);
            size = math<float>::floor( math<float>::log(size) * log2base );
            size = math<int>::pow(2, size);
            if( size <= 0 )
                throw new Exception();
            ((ciUITextInput *)gui->getWidget("size"))->setTextString(toString(size));
            resizePackage(size);
            
            int scaler = math<int>::min( 100 * 512.f / size, 100);
            ((ciUILabel *)gui->getWidget("info"))->setLabel("size: " + toString(scaler) + "%");
            
            ((ciUILabel *)gui->getWidget("action1"))->setLabel("Success: ");
            ((ciUILabel *)gui->getWidget("action2"))->setLabel("Bin has been re-");
            ((ciUILabel *)gui->getWidget("action3"))->setLabel("sized to ");
            ((ciUILabel *)gui->getWidget("action4"))->setLabel(toString(size) + "x" + toString(size));
            ((ciUILabel *)gui->getWidget("action5"))->setLabel("");
            ((ciUILabel *)gui->getWidget("action6"))->setLabel("");
            
        }catch( ... ){
            ((ciUITextInput *)gui->getWidget("size"))->setTextString(toString(mBinPackerSingle.getWidth()));
            
            int scaler = math<int>::min( 100 * 512.f / mBinPackerSingle.getWidth(), 100);
            ((ciUILabel *)gui->getWidget("info"))->setLabel("size: " + toString(scaler) + "%");
            
            ((ciUILabel *)gui->getWidget("action1"))->setLabel("Error: ");
            ((ciUILabel *)gui->getWidget("action2"))->setLabel("can't get correct");
            ((ciUILabel *)gui->getWidget("action3"))->setLabel("bin size. Abort.");
            ((ciUILabel *)gui->getWidget("action4"))->setLabel("");
            ((ciUILabel *)gui->getWidget("action5"))->setLabel("");
            ((ciUILabel *)gui->getWidget("action6"))->setLabel("");
        }
    }
}

void AtlasGeneratorApp::clear(){
    mUnpackedArea.clear();
    mPackedArea.clear();
    mTexSrc.clear();
    mTexName.clear();
    
    mBinPackerSingle.setSize( 128, 128 );
	mBinPackerMulti.setSize(  128, 128 );
    
    ((ciUITextInput *)gui->getWidget("size"))->setTextString(toString(128));
    ((ciUILabel *)gui->getWidget("info"))->setLabel("size: 100%");
    ((ciUILabel *)gui->getWidget("action2"))->setLabel("All textures have");
    ((ciUILabel *)gui->getWidget("action3"))->setLabel("been cleared.");
    ((ciUILabel *)gui->getWidget("action4"))->setLabel("");
    ((ciUILabel *)gui->getWidget("action5"))->setLabel("");
    ((ciUILabel *)gui->getWidget("action6"))->setLabel("");
    ((ciUILabel *)gui->getWidget("action1"))->setLabel("Success:");
    
}

void AtlasGeneratorApp::shutdown(){
    delete gui;
}

void AtlasGeneratorApp::resizePackage(int w){
    mBinPackerSingle.setSize( w, w );
	mBinPackerMulti.setSize(  w, w );
    doPack();
}

bool AtlasGeneratorApp::addFolder(){
    fs::path path = getFolderPath();
    
    if( ! path.empty() ){
        std::vector<string> extensions = ImageIo::getLoadExtensions();
        fs::directory_iterator it(path.string()), eod;
        for( ; it != eod; ++it ){
            if(!is_directory(*it)){
                //make sure we can read this image file
                string extension = it->path().extension().string();
                extension = extension.substr(1, extension.size());
                if( std::find(extensions.begin(), extensions.end(), extension) != extensions.end() ){
                    string filename = it->path().filename().string();
                    //make we don't have this already in the list
                    if( std::find (mTexName.begin(), mTexName.end(), filename) == mTexName.end()){
                        gl::TextureRef  tex     = gl::Texture::create(loadImage(loadFile(it->path())));
                        Area            area    = tex->getBounds();
                        mUnpackedArea.push_back(area    );
                        mTexSrc.push_back(      tex     );
                        mTexName.push_back(     filename);
                    }
                }
            }
        }
        
        //hacking the timer to solve the ciUI related bug
        hackTimer.start();
        return true;
    }
    
    //hacking the timer to solve the ciUI related bug
    hackTimer.start();
    return false;
}

bool AtlasGeneratorApp::addTexture(){
    fs::path path = getOpenFilePath("", ImageIo::getLoadExtensions());
    
    if( ! path.empty() ) {
        
        string filename = path.filename().string();
        std::vector<string>::iterator it;
        
        // iterator to vector element:
        it = std::find (mTexName.begin(), mTexName.end(), filename);
        if( it != mTexName.end() ){
            ((ciUILabel *)gui->getWidget("action1"))->setLabel("Error: ");
            ((ciUILabel *)gui->getWidget("action2"))->setLabel(mTexName.back());
            ((ciUILabel *)gui->getWidget("action3"))->setLabel("already exists.");
            ((ciUILabel *)gui->getWidget("action4"))->setLabel("File with same");
            ((ciUILabel *)gui->getWidget("action5"))->setLabel("name can't be");
            ((ciUILabel *)gui->getWidget("action6"))->setLabel("added twice.");
            //hacking the timer to solve the ciUI related bug
            hackTimer.start();
            return false;
        }else{
            gl::TextureRef  tex     = gl::Texture::create(loadImage(loadFile(path)));
            Area            area    = tex->getBounds();
            mUnpackedArea.push_back(area                    );
            mTexSrc.push_back(      tex                     );
            mTexName.push_back(     filename                );
            //hacking the timer to solve the ciUI related bug
            hackTimer.start();
            return true;
        }
    }
    //hacking the timer to solve the ciUI related bug
    hackTimer.start();
    return false;
}

void AtlasGeneratorApp::exportPackage(){
    fs::path path = getSaveFilePath("", ImageIo::getWriteExtensions());
    if( ! path.empty() ) {
        
        switch (mMode) {
            case SINGLE:
                {
                    gl::Fbo::Format msaaFormat;
                    msaaFormat.setSamples( 4 );
                    msaaFormat.enableDepthBuffer(false);
                    gl::Fbo mBuffer = gl::Fbo( mBinPackerSingle.getWidth(), mBinPackerSingle.getHeight(), msaaFormat );

                    //export xml file
                    XmlTree absolutelibrary( "atlas", "" );
                    XmlTree relativelibrary( "atlas", "" );
                    
                    //render all texture for output
                    mBuffer.bindFramebuffer();
                    int w = mBuffer.getWidth();
                    gl::setViewport(Area(0, 0, w, w));
                    gl::clear(ColorA(0,0,0,0));
                    gl::pushMatrices();
                    gl::setMatricesWindow(w,w,mFlipped);
                    gl::color(1, 1, 1);
                    for(unsigned i=0;i<mPackedArea.size();++i) {
                        mTexSrc[i]->enableAndBind();
                        gl::drawSolidRect( mPackedArea[i] );
                        mTexSrc[i]->unbind();
                        
                        //parse area size and position
                        XmlTree absolute( "texture", "" );
                        absolute.push_back( XmlTree( "name", mTexName[i]                            ));
                        absolute.push_back( XmlTree( "x",    toString(mPackedArea[i].getX1())       ));
                        absolute.push_back( XmlTree( "y",    toString(mPackedArea[i].getY1())       ));
                        absolute.push_back( XmlTree( "w",    toString(mPackedArea[i].getWidth())    ));
                        absolute.push_back( XmlTree( "h",    toString(mPackedArea[i].getHeight())   ));
                        absolutelibrary.push_back( absolute );
                        
                        XmlTree relative( "texture", "" );
                        relative.push_back( XmlTree( "name", mTexName[i]                                    ));
                        relative.push_back( XmlTree( "x",    toString(1.f * mPackedArea[i].getX1() / w)     ));
                        relative.push_back( XmlTree( "y",    toString(1.f * mPackedArea[i].getY1() / w)     ));
                        relative.push_back( XmlTree( "w",    toString(1.f * mPackedArea[i].getWidth() / w)  ));
                        relative.push_back( XmlTree( "h",    toString(1.f * mPackedArea[i].getHeight() / w) ));
                        relativelibrary.push_back( relative );
                    }
                    
                    gl::popMatrices();
                    mBuffer.unbindFramebuffer();
                    gl::setViewport(getWindowBounds());
                    
                    //save image to destination path
                    writeImage(path.string(), mBuffer.getTexture());
                    //save xml to destination path
                    string filepath     = path.string();
                    string extension    = path.extension().string();
                    filepath = filepath.substr(0, filepath.size() - extension.size());
                    absolutelibrary.write( writeFile( filepath + "_absolute.xml" ) );
                    relativelibrary.write( writeFile( filepath + "_relative.xml" ) );
                    mBuffer.reset();
                }
                break;
            case MULTI:
                {
                    //grab all bin indices
                    vector<int> binIndex;
                    for(unsigned i=0;i<mPackedArea.size();++i)
                        binIndex.push_back(mPackedArea[i].getBin());
                    
                    //remove duplicated ones
                    sort( binIndex.begin(), binIndex.end() );
                    binIndex.erase( unique( binIndex.begin(), binIndex.end() ), binIndex.end() );
                    
                    gl::Fbo::Format msaaFormat;
                    msaaFormat.setSamples( 4 );
                    msaaFormat.enableDepthBuffer(false);
                    gl::Fbo mBuffer = gl::Fbo( mBinPackerSingle.getWidth(), mBinPackerSingle.getHeight(), msaaFormat );
                    int w = mBuffer.getWidth();
                    
                    gl::color(1, 1, 1);
                    for(unsigned i=0; i<binIndex.size();++i) {
                        int bin = binIndex[i];
                    
                        //export xml file
                        XmlTree absolutelibrary( "atlas", "" );
                        XmlTree relativelibrary( "atlas", "" );
                        
                        //render textures belong to this bin for output
                        gl::setViewport(Area(0, 0, w, w));
                        mBuffer.bindFramebuffer();
                        gl::clear(ColorA(0,0,0,0));
                        gl::pushMatrices();
                        gl::setMatricesWindow(w,w,mFlipped);
                        for( unsigned j=0; j < mPackedArea.size(); j++ ){
                            if( mPackedArea[j].getBin() == bin ){
                                mTexSrc[j]->enableAndBind();
                                gl::drawSolidRect( mPackedArea[j] );
                                mTexSrc[j]->unbind();
                            
                                //parse area size and position
                                XmlTree absolute( "texture", "" );
                                absolute.push_back( XmlTree( "name", mTexName[j]                            ));
                                absolute.push_back( XmlTree( "x",    toString(mPackedArea[j].getX1())       ));
                                absolute.push_back( XmlTree( "y",    toString(mPackedArea[j].getY1())       ));
                                absolute.push_back( XmlTree( "w",    toString(mPackedArea[j].getWidth())    ));
                                absolute.push_back( XmlTree( "h",    toString(mPackedArea[j].getHeight())   ));
                                absolutelibrary.push_back( absolute );
                                
                                XmlTree relative( "texture", "" );
                                relative.push_back( XmlTree( "name", mTexName[j]                                    ));
                                relative.push_back( XmlTree( "x",    toString(1.f * mPackedArea[j].getX1() / w)     ));
                                relative.push_back( XmlTree( "y",    toString(1.f * mPackedArea[j].getY1() / w)     ));
                                relative.push_back( XmlTree( "w",    toString(1.f * mPackedArea[j].getWidth() / w)  ));
                                relative.push_back( XmlTree( "h",    toString(1.f * mPackedArea[j].getHeight() / w) ));
                                relativelibrary.push_back( relative );
                            }
                        }
                        gl::popMatrices();
                        mBuffer.unbindFramebuffer();
                        gl::setViewport(getWindowBounds());
                        
                        //save xml to destination path
                        string filepath     = path.string();
                        string extension    = path.extension().string();
                        filepath            = filepath.substr(0, filepath.size() - extension.size());
                        
                        //save image to destination path
                        writeImage(filepath + "_" + toString(bin+1) + extension, mBuffer.getTexture());
                        absolutelibrary.write( writeFile( filepath + "_" + toString(bin+1) + "_absolute.xml" ) );
                        relativelibrary.write( writeFile( filepath + "_" + toString(bin+1) + "_relative.xml" ) );
                    }
                    
                    mBuffer.reset();
                }
                break;
        }
    }
    //hacking the timer to solve the ciUI related bug
    hackTimer.start();
}

void AtlasGeneratorApp::doPack(){
    switch( mMode ){
        case SINGLE:
            {
                // show the total number of Area's in the window title bar
                getWindow()->setTitle( "AtlasGenerator | Single Bin | " + toString(mUnpackedArea.size()) );
                bool finished = false;
                while( !finished ){
                    try{
                        // mPacked will contain all Area's of mUnpacked in the exact same order,
                        // but moved to a different spot in the bin and represented as a BinnedArea.
                        // BinnedAreas can be used directly as Areas, conversion will happen automatically.
                        // Unpacked will not be altered.
                        mPackedArea = mBinPackerSingle.pack( mUnpackedArea );
                        finished = true;
                        
                        if( mTexName.size() > 0 ){
                            ((ciUILabel *)gui->getWidget("action1"))->setLabel("Sucess: ");
                            ((ciUILabel *)gui->getWidget("action2"))->setLabel(mTexName.back());
                            ((ciUILabel *)gui->getWidget("action3"))->setLabel("has been added");
                            ((ciUILabel *)gui->getWidget("action4"))->setLabel("to atlas.");
                            ((ciUILabel *)gui->getWidget("action5"))->setLabel("");
                            ((ciUILabel *)gui->getWidget("action6"))->setLabel("");
                        }
                        
                    }catch(...){
                        // the bin is not large enough to contain all Area's, so let's
                        // double the size...
                        int size = mBinPackerSingle.getWidth() << 1;
                        console() << "resize to: " << size << endl;
                        resizePackage(size);
                    }
                }
                ((ciUITextInput *)gui->getWidget("size"))->setTextString(toString(mBinPackerSingle.getWidth()));
            }
            break;
        case MULTI:
            {
                // show the total number of Area's in the window title bar
                getWindow()->setTitle( "AtlasGenerator | Multi Bin | " + toString(mUnpackedArea.size()) );
                try{
                    //  mPacked will contain all Area's of mUnpacked in the exact same order,
                    // but moved to a different spot in the bin and represented as a BinnedArea.
                    // BinnedAreas can be used directly as Areas, conversion will happen automatically.
                    // Use the BinnedArea::getBin() method to find out to which bin the Area belongs.
                    // Unpacked will not be altered.
                    mPackedArea = mBinPackerMulti.pack( mUnpackedArea );
                    
                    if( mTexName.size() > 0 ){
                        ((ciUILabel *)gui->getWidget("action1"))->setLabel("Sucess: ");
                        ((ciUILabel *)gui->getWidget("action2"))->setLabel(mTexName.back());
                        ((ciUILabel *)gui->getWidget("action3"))->setLabel("has been added");
                        ((ciUILabel *)gui->getWidget("action4"))->setLabel("to atlas.");
                        ((ciUILabel *)gui->getWidget("action5"))->setLabel("");
                        ((ciUILabel *)gui->getWidget("action6"))->setLabel("");
                    }
                    
                }catch(...){
                    ((ciUILabel *)gui->getWidget("action1"))->setLabel("Error: ");
                    ((ciUILabel *)gui->getWidget("action2"))->setLabel("texture is too big");
                    ((ciUILabel *)gui->getWidget("action3"))->setLabel("for one bin to ");
                    ((ciUILabel *)gui->getWidget("action4"))->setLabel("handle, please ");
                    ((ciUILabel *)gui->getWidget("action5"))->setLabel("consider enlarge");
                    ((ciUILabel *)gui->getWidget("action6"))->setLabel("bin size.");
                }
            }
            break;
	}
}

void AtlasGeneratorApp::update(){
}

void AtlasGeneratorApp::draw(){
	// clear out the window with black
	gl::clear( Color( .4f, .4f, .4f ) );
    
    gl::pushMatrices();
    gl::translate(128.f, 0.f);
    
    float scaler =  math<float>::clamp(512.f / mBinPackerSingle.getWidth());
    if( scaler < 1.f )
        gl::scale( scaler, scaler );
    
    switch( mMode )
	{
        case SINGLE:
            // draw the borders of the bin
            gl::color( Color( 0, 1, 1 ) );
            gl::drawStrokedRect( Rectf( Vec2f::zero(), mBinPackerSingle.getSize() ) );
    
            // draw the binned rectangles
            gl::color(1, 1, 1);
            for(unsigned i=0;i<mPackedArea.size();++i) {
                mTexSrc[i]->enableAndBind();
                gl::drawSolidRect( mPackedArea[i] );
                mTexSrc[i]->unbind();
            }
            break;
        case MULTI:{
                unsigned n = (unsigned) floor( 512 / (float) (scaler * mBinPackerMulti.getWidth()) );
                for(unsigned i=0;i<mPackedArea.size();++i) {
                    int bin = mPackedArea[i].getBin();
                    gl::pushMatrices();
                    gl::translate( (float) ((bin % n) * scaler * mBinPackerMulti.getWidth()), (float) ((bin / n) * scaler * mBinPackerMulti.getHeight()), 0.0f );
                    
                    // draw the binned rectangle
                    gl::color( Color( 0, 1, 1 ) );
                    gl::drawStrokedRect( Rectf( Vec2f::zero(), mBinPackerMulti.getSize() ) );
                    
                    gl::color( Color( 1, 1, 1 ) );
                    mTexSrc[i]->enableAndBind();
                    gl::drawSolidRect( mPackedArea[i] );
                    mTexSrc[i]->unbind();
            	
                    gl::popMatrices();
                }
            }
            break;
	}
    gl::popMatrices();
    
    gui->draw();
}

CINDER_APP_NATIVE( AtlasGeneratorApp, RendererGl )
