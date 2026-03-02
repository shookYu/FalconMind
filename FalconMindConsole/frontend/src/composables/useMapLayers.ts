import { ref, computed, type Ref } from 'vue';

export type LayerType = 'satellite' | 'terrain' | 'street' | 'dark';

export interface MapLayer {
  id: LayerType;
  name: string;
  icon: string;
  provider: any;
}

export function useMapLayers(viewerRef: Ref<any>) {
  const Cesium = computed(() => (window as any).Cesium);
  
  const currentLayer = ref<LayerType>('street');
  const opacity = ref(1.0);
  
  // Layer definitions
  const layers: Record<LayerType, MapLayer> = {
    street: {
      id: 'street',
      name: '街道地图',
      icon: 'MapLocation',
      provider: null // Will be created on demand
    },
    satellite: {
      id: 'satellite',
      name: '卫星影像',
      icon: 'Picture',
      provider: null
    },
    terrain: {
      id: 'terrain',
      name: '地形图',
      icon: 'Mount',
      provider: null
    },
    dark: {
      id: 'dark',
      name: '暗色模式',
      icon: 'Moon',
      provider: null
    }
  };
  
  // Create imagery provider for each layer type
  const createImageryProvider = (type: LayerType): any => {
    switch (type) {
      case 'satellite':
        // Bing Maps Satellite (requires API key)
        return new Cesium.value.BingMapsImageryProvider({
          key: import.meta.env.VITE_BING_MAPS_KEY || '',
          mapStyle: Cesium.value.BingMapsStyle.AERIAL
        });
        
      case 'terrain':
        // OpenTopoMap
        return new Cesium.value.UrlTemplateImageryProvider({
          url: 'https://{s}.tile.opentopomap.org/{z}/{x}/{y}.png',
          subdomains: ['a', 'b', 'c'],
          maximumLevel: 17,
          credit: 'Map data: © OpenStreetMap contributors, SRTM | Map style: © OpenTopoMap'
        });
        
      case 'dark':
        // CartoDB Dark Matter
        return new Cesium.value.UrlTemplateImageryProvider({
          url: 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
          subdomains: ['a', 'b', 'c', 'd'],
          maximumLevel: 19,
          credit: '© OpenStreetMap contributors © CARTO'
        });
        
      case 'street':
      default:
        // OpenStreetMap
        return new Cesium.value.UrlTemplateImageryProvider({
          url: 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
          subdomains: ['a', 'b', 'c'],
          maximumLevel: 18,
          credit: '© OpenStreetMap contributors'
        });
    }
  };
  
  // Current layer name
  const currentLayerName = computed(() => layers[currentLayer.value].name);
  
  // Available layers list
  const availableLayers = computed(() => 
    Object.values(layers).map(layer => ({
      id: layer.id,
      name: layer.name,
      icon: layer.icon,
      active: layer.id === currentLayer.value
    }))
  );
  
  // Switch to a different layer
  const switchLayer = async (type: LayerType) => {
    if (!viewerRef.value || type === currentLayer.value) return;
    
    try {
      const viewer = viewerRef.value;
      
      // Create new imagery provider
      const newProvider = createImageryProvider(type);
      
      // Remove existing base layers
      viewer.imageryLayers.removeAll();
      
      // Add new base layer
      viewer.imageryLayers.addImageryProvider(newProvider);
      
      // Update current layer
      currentLayer.value = type;
      
      // Update layer reference
      layers[type].provider = newProvider;
      
    } catch (error) {
      console.error('Failed to switch map layer:', error);
    }
  };
  
  // Set layer opacity
  const setOpacity = (value: number) => {
    if (!viewerRef.value) return;
    
    opacity.value = Math.max(0, Math.min(1, value));
    
    // Get the base imagery layer (index 0)
    const baseLayer = viewerRef.value.imageryLayers.get(0);
    if (baseLayer) {
      baseLayer.alpha = opacity.value;
    }
  };
  
  // Toggle between satellite and street
  const toggleSatellite = () => {
    const newType = currentLayer.value === 'satellite' ? 'street' : 'satellite';
    switchLayer(newType);
  };
  
  // Initialize layer
  const initializeLayer = () => {
    if (!viewerRef.value) return;
    
    // The base layer is already set during viewer initialization
    // Just store the reference
    const baseLayer = viewerRef.value.imageryLayers.get(0);
    if (baseLayer) {
      layers.street.provider = baseLayer.imageryProvider;
    }
  };
  
  // Add overlay layer (e.g., weather, airspace)
  const addOverlayLayer = (url: string, options: {
    alpha?: number;
    show?: boolean;
    maximumLevel?: number;
  } = {}) => {
    if (!viewerRef.value) return null;
    
    const provider = new Cesium.value.UrlTemplateImageryProvider({
      url,
      maximumLevel: options.maximumLevel || 18
    });
    
    const layer = viewerRef.value.imageryLayers.addImageryProvider(provider);
    
    if (options.alpha !== undefined) {
      layer.alpha = options.alpha;
    }
    if (options.show !== undefined) {
      layer.show = options.show;
    }
    
    return layer;
  };
  
  // Remove overlay layer
  const removeOverlayLayer = (layer: any) => {
    if (!viewerRef.value || !layer) return;
    
    viewerRef.value.imageryLayers.remove(layer);
  };
  
  return {
    // State
    currentLayer,
    currentLayerName,
    opacity,
    availableLayers,
    
    // Actions
    switchLayer,
    setOpacity,
    toggleSatellite,
    initializeLayer,
    addOverlayLayer,
    removeOverlayLayer
  };
}
