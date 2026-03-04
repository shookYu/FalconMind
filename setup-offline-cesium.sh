#!/bin/bash
# Setup script for FalconMind offline Cesium
# Run this script to install Cesium and download map tiles

echo "=========================================="
echo "FalconMind Offline Cesium Setup"
echo "=========================================="
echo ""

# Function to setup Console project
setup_console() {
  echo "Setting up FalconMindConsole..."
  cd /home/shook/study/opencode/FalconMindConsole/frontend
  
  # Install Cesium
  echo "Installing Cesium..."
  npm install cesium
  
  # Copy Cesium files
  echo "Copying Cesium files to public/cesium/..."
  mkdir -p public/cesium
  cp -r node_modules/cesium/Build/Cesium/* public/cesium/
  
  echo "✅ Console Cesium setup complete!"
  echo ""
}

# Function to setup Builder project
setup_builder() {
  echo "Setting up FalconMindBuilder..."
  cd /home/shook/study/opencode/FalconMindBuilder/frontend
  
  # Install Cesium
  echo "Installing Cesium..."
  npm install cesium
  
  # Copy Cesium files
  echo "Copying Cesium files to public/cesium/..."
  mkdir -p public/cesium
  cp -r node_modules/cesium/Build/Cesium/* public/cesium/
  
  echo "✅ Builder Cesium setup complete!"
  echo ""
}

# Function to download map tiles
download_maps() {
  echo "Downloading Changping Park map tiles..."
  echo "This will download ~123 tiles (about 6-12 MB)"
  echo ""
  
  # Download for Console
  if [ -f "/home/shook/study/opencode/FalconMindConsole/frontend/public/download-map-tiles.py" ]; then
    cd /home/shook/study/opencode/FalconMindConsole/frontend/public
    python3 download-map-tiles.py
  fi
  
  # Copy to Builder
  echo ""
  echo "Copying map tiles to Builder..."
  mkdir -p /home/shook/study/opencode/FalconMindBuilder/frontend/public/map-tiles/changping-park
  cp -r /home/shook/study/opencode/FalconMindConsole/frontend/public/map-tiles/changping-park/* \
        /home/shook/study/opencode/FalconMindBuilder/frontend/public/map-tiles/changping-park/ 2>/dev/null || echo "Map tiles already exist in Builder"
  
  echo "✅ Map tiles setup complete!"
}

# Main menu
echo "What would you like to do?"
echo ""
echo "1) Setup both projects (Console + Builder)"
echo "2) Setup Console only"
echo "3) Setup Builder only"
echo "4) Download map tiles only"
echo "5) Exit"
echo ""
read -p "Enter your choice (1-5): " choice

case $choice in
  1)
    setup_console
    setup_builder
    download_maps
    echo ""
    echo "=========================================="
    echo "✅ All done! Both projects are ready."
    echo "=========================================="
    echo ""
    echo "Next steps:"
    echo "1. cd FalconMindConsole/frontend && npm run dev"
    echo "2. cd FalconMindBuilder/frontend && npm run dev"
    ;;
  2)
    setup_console
    download_maps
    echo ""
    echo "✅ Console setup complete!"
    ;;
  3)
    setup_builder
    echo ""
    echo "⚠️  Don't forget to download map tiles in Console project first!"
    ;;
  4)
    download_maps
    echo ""
    echo "✅ Map tiles downloaded!"
    ;;
  5)
    echo "Goodbye!"
    exit 0
    ;;
  *)
    echo "Invalid choice"
    exit 1
    ;;
esac

echo ""
echo "Verification:"
echo "- Console Cesium: $(ls /home/shook/study/opencode/FalconMindConsole/frontend/public/cesium/Cesium.js 2>/dev/null && echo '✅' || echo '❌')"
echo "- Console Map Tiles: $(ls /home/shook/study/opencode/FalconMindConsole/frontend/public/map-tiles/changping-park/12/ 2>/dev/null | wc -l) tiles"
echo "- Builder Cesium: $(ls /home/shook/study/opencode/FalconMindBuilder/frontend/public/cesium/Cesium.js 2>/dev/null && echo '✅' || echo '❌')"
