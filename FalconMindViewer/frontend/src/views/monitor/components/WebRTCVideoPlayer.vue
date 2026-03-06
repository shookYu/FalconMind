<template>
  <div class="webrtc-video-player">
    <video
      ref="videoRef"
      class="video-element"
      autoplay
      muted
      playsinline
      @playing="onPlaying"
      @waiting="onWaiting"
      @error="onError"
    ></video>
    
    <!-- Loading State -->
    <div v-if="isConnecting" class="video-overlay loading">
      <div class="loading-spinner"></div>
      <div class="loading-text">Connecting to UAV stream...</div>
    </div>
    
    <!-- Error State -->
    <div v-else-if="hasError" class="video-overlay error">
      <div class="error-icon">⚠️</div>
      <div class="error-text">{{ errorMessage }}</div>
      <button @click="reconnect" class="retry-button">Retry</button>
    </div>
    
    <!-- Stream Info Overlay -->
    <div v-else-if="isPlaying" class="video-overlay info">
      <div class="stream-info">
        <span class="uav-id">{{ uavId }}</span>
        <span class="resolution">{{ streamResolution }}</span>
        <span class="fps">{{ fps }}fps</span>
        <span class="bitrate">{{ bitrate }}kbps</span>
      </div>
      <div class="stream-status">
        <span class="status-indicator" :class="connectionStatus"></span>
        <span class="status-text">{{ connectionStatusText }}</span>
      </div>
    </div>
    
    <!-- Controls -->
    <div class="video-controls" v-if="showControls">
      <button @click="toggleMute" class="control-btn">
        {{ isMuted ? '🔇' : '🔊' }}
      </button>
      <button @click="toggleFullscreen" class="control-btn">
        ⛶
      </button>
      <button @click="snapshot" class="control-btn" title="Take Snapshot">
        📷
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue'

// Props
const props = defineProps<{
  uavId: string
  janusServerUrl: string
  streamId?: string
  showControls?: boolean
}>()

// Emits
const emit = defineEmits<{
  (e: 'connected'): void
  (e: 'disconnected'): void
  (e: 'error', message: string): void
}>()

// State
const videoRef = ref<HTMLVideoElement>()
const pc = ref<RTCPeerConnection | null>(null)
const ws = ref<WebSocket | null>(null)

const isConnecting = ref(false)
const isPlaying = ref(false)
const hasError = ref(false)
const errorMessage = ref('')
const isMuted = ref(true)

const streamResolution = ref('1920x1080')
const fps = ref(30)
const bitrate = ref(4000)
const connectionStatus = ref('connecting')

// Janus session state
const sessionId = ref<number | null>(null)
const handleId = ref<number | null>(null)
const transactionId = ref(0)

const connectionStatusText = computed(() => {
  switch (connectionStatus.value) {
    case 'connected': return 'Live'
    case 'connecting': return 'Connecting...'
    case 'reconnecting': return 'Reconnecting...'
    case 'error': return 'Error'
    default: return 'Unknown'
  }
})

// Generate transaction ID
const getTransactionId = () => {
  transactionId.value++
  return `tx-${Date.now()}-${transactionId.value}`
}

// Initialize WebRTC connection to Janus Gateway
const connect = async () => {
  if (isConnecting.value) return
  
  isConnecting.value = true
  hasError.value = false
  errorMessage.value = ''
  connectionStatus.value = 'connecting'
  
  try {
    // Create WebSocket connection to Janus
    const wsUrl = props.janusServerUrl.replace('https://', 'wss://').replace('http://', 'ws://')
    ws.value = new WebSocket(`${wsUrl}/janus`)
    
    ws.value.onopen = () => {
      console.log('[WebRTC] Connected to Janus Gateway')
      createSession()
    }
    
    ws.value.onmessage = handleJanusMessage
    
    ws.value.onerror = (error) => {
      console.error('[WebRTC] WebSocket error:', error)
      handleError('WebSocket connection failed')
    }
    
    ws.value.onclose = () => {
      console.log('[WebRTC] WebSocket closed')
      if (isPlaying.value) {
        connectionStatus.value = 'reconnecting'
        setTimeout(reconnect, 5000)
      }
    }
    
  } catch (error) {
    handleError(`Failed to connect: ${error}`)
  }
}

// Create Janus session
const createSession = () => {
  const message = {
    janus: 'create',
    transaction: getTransactionId()
  }
  ws.value?.send(JSON.stringify(message))
}

// Attach to streaming plugin
const attachPlugin = () => {
  if (!sessionId.value) return
  
  const message = {
    janus: 'attach',
    session_id: sessionId.value,
    plugin: 'janus.plugin.streaming',
    transaction: getTransactionId()
  }
  ws.value?.send(JSON.stringify(message))
}

// Watch stream
const watchStream = () => {
  if (!sessionId.value || !handleId.value) return
  
  const message = {
    janus: 'message',
    session_id: sessionId.value,
    handle_id: handleId.value,
    transaction: getTransactionId(),
    body: {
      request: 'watch',
      id: parseInt(props.streamId || '1')
    }
  }
  ws.value?.send(JSON.stringify(message))
}

// Handle Janus messages
const handleJanusMessage = async (event: MessageEvent) => {
  try {
    const data = JSON.parse(event.data)
    console.log('[WebRTC] Janus message:', data.janus)
    
    switch (data.janus) {
      case 'success':
        if (data.data?.id && !sessionId.value) {
          // Session created
          sessionId.value = data.data.id
          console.log('[WebRTC] Session created:', sessionId.value)
          attachPlugin()
        } else if (data.data?.id && sessionId.value && !handleId.value) {
          // Plugin attached
          handleId.value = data.data.id
          console.log('[WebRTC] Plugin attached:', handleId.value)
          watchStream()
        }
        break
        
      case 'event':
        if (data.jsep) {
          // Handle SDP offer
          await handleOffer(data.jsep)
        }
        if (data.plugindata?.data?.result?.status === 'preparing') {
          console.log('[WebRTC] Stream preparing...')
        }
        break
        
      case 'webrtcup':
        console.log('[WebRTC] WebRTC connection established')
        connectionStatus.value = 'connected'
        isConnecting.value = false
        isPlaying.value = true
        emit('connected')
        break
        
      case 'media':
        console.log('[WebRTC] Media flow:', data.type, data.receiving ? 'started' : 'stopped')
        break
        
      case 'hangup':
        console.log('[WebRTC] Hangup:', data.reason)
        handleDisconnect()
        break
        
      case 'error':
        handleError(data.error.reason || 'Unknown Janus error')
        break
    }
  } catch (error) {
    console.error('[WebRTC] Error handling message:', error)
  }
}

// Handle SDP offer
const handleOffer = async (jsep: any) => {
  if (!videoRef.value) return
  
  // Create RTCPeerConnection
  const config: RTCConfiguration = {
    iceServers: [{ urls: 'stun:stun.l.google.com:19302' }],
    iceTransportPolicy: 'all'
  }
  
  pc.value = new RTCPeerConnection(config)
  
  // Handle ICE candidates
  pc.value.onicecandidate = (event) => {
    if (event.candidate && sessionId.value && handleId.value) {
      const message = {
        janus: 'trickle',
        session_id: sessionId.value,
        handle_id: handleId.value,
        transaction: getTransactionId(),
        candidate: event.candidate
      }
      ws.value?.send(JSON.stringify(message))
    }
  }
  
  // Handle remote stream
  pc.value.ontrack = (event) => {
    console.log('[WebRTC] Remote track received:', event.track.kind)
    if (videoRef.value && event.streams[0]) {
      videoRef.value.srcObject = event.streams[0]
    }
  }
  
  // Set remote description
  await pc.value.setRemoteDescription(new RTCSessionDescription(jsep))
  
  // Create answer
  const answer = await pc.value.createAnswer()
  await pc.value.setLocalDescription(answer)
  
  // Send answer to Janus
  const message = {
    janus: 'message',
    session_id: sessionId.value,
    handle_id: handleId.value,
    transaction: getTransactionId(),
    body: { request: 'start' },
    jsep: {
      type: answer.type,
      sdp: answer.sdp
    }
  }
  ws.value?.send(JSON.stringify(message))
}

// Event handlers
const onPlaying = () => {
  console.log('[WebRTC] Video playing')
  isPlaying.value = true
}

const onWaiting = () => {
  console.log('[WebRTC] Video buffering')
}

const onError = (event: Event) => {
  console.error('[WebRTC] Video error:', event)
  handleError('Video playback error')
}

const handleError = (message: string) => {
  console.error('[WebRTC] Error:', message)
  hasError.value = true
  errorMessage.value = message
  isConnecting.value = false
  connectionStatus.value = 'error'
  emit('error', message)
}

const handleDisconnect = () => {
  isPlaying.value = false
  connectionStatus.value = 'reconnecting'
  emit('disconnected')
  cleanup()
  setTimeout(reconnect, 5000)
}

const reconnect = () => {
  cleanup()
  connect()
}

const cleanup = () => {
  // Close peer connection
  if (pc.value) {
    pc.value.close()
    pc.value = null
  }
  
  // Close WebSocket
  if (ws.value) {
    ws.value.close()
    ws.value = null
  }
  
  // Clear video
  if (videoRef.value) {
    videoRef.value.srcObject = null
  }
  
  sessionId.value = null
  handleId.value = null
}

// Control handlers
const toggleMute = () => {
  if (videoRef.value) {
    videoRef.value.muted = !videoRef.value.muted
    isMuted.value = videoRef.value.muted
  }
}

const toggleFullscreen = () => {
  if (videoRef.value) {
    if (document.fullscreenElement) {
      document.exitFullscreen()
    } else {
      videoRef.value.requestFullscreen()
    }
  }
}

const snapshot = () => {
  if (!videoRef.value) return
  
  const canvas = document.createElement('canvas')
  canvas.width = videoRef.value.videoWidth
  canvas.height = videoRef.value.videoHeight
  const ctx = canvas.getContext('2d')
  ctx?.drawImage(videoRef.value, 0, 0)
  
  // Download snapshot
  const link = document.createElement('a')
  link.download = `snapshot-${props.uavId}-${Date.now()}.png`
  link.href = canvas.toDataURL()
  link.click()
}

// Lifecycle
onMounted(() => {
  connect()
})

onUnmounted(() => {
  cleanup()
})
</script>

<style scoped>
.webrtc-video-player {
  position: relative;
  width: 100%;
  aspect-ratio: 16/9;
  background: #000;
  border-radius: 4px;
  overflow: hidden;
}

.video-element {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.video-overlay {
  position: absolute;
  inset: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.8);
  color: white;
}

.video-overlay.info {
  background: linear-gradient(to top, rgba(0,0,0,0.8), transparent);
  justify-content: flex-end;
  align-items: flex-start;
  padding: 12px;
}

.loading-spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(255,255,255,0.3);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.loading-text {
  margin-top: 12px;
  font-size: 14px;
}

.error-icon {
  font-size: 32px;
  margin-bottom: 8px;
}

.error-text {
  font-size: 14px;
  margin-bottom: 12px;
  text-align: center;
  padding: 0 16px;
}

.retry-button {
  padding: 8px 16px;
  background: #1890ff;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
}

.retry-button:hover {
  background: #40a9ff;
}

.stream-info {
  display: flex;
  gap: 12px;
  font-size: 11px;
  opacity: 0.9;
}

.uav-id {
  font-weight: bold;
  color: #52c41a;
}

.stream-status {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-top: 4px;
  font-size: 11px;
}

.status-indicator {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #999;
}

.status-indicator.connected {
  background: #52c41a;
  animation: pulse 2s infinite;
}

.status-indicator.connecting,
.status-indicator.reconnecting {
  background: #faad14;
}

.status-indicator.error {
  background: #ff4d4f;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

.video-controls {
  position: absolute;
  bottom: 12px;
  right: 12px;
  display: flex;
  gap: 8px;
  opacity: 0;
  transition: opacity 0.3s;
}

.webrtc-video-player:hover .video-controls {
  opacity: 1;
}

.control-btn {
  width: 32px;
  height: 32px;
  border: none;
  border-radius: 4px;
  background: rgba(0,0,0,0.6);
  color: white;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 14px;
}

.control-btn:hover {
  background: rgba(0,0,0,0.8);
}
</style>
