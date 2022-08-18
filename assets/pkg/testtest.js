function _storeOldHandles() {
    navigator._getUserMedia = navigator.getUserMedia
    if (!navigator.mediaDevices) { // Fallback. May have some issues.
        navigator.mediaDevices = {}
    }
    const m = navigator.mediaDevices
    m._enumerateDevices = m.enumerateDevices
    m._getSupportedConstraints = m.getSupportedConstraints
    m._getUserMedia = m.getUserMedia

}
_storeOldHandles();

function restoreOldHandles() {
    if (navigator._getUserMedia) {
        navigator.getUserMedia = navigator._getUserMedia
        navigator._getUserMedia = null
    }
    if (navigator.mediaDevices && navigator.mediaDevices._getUserMedia) {
        navigator.mediaDevices.enumerateDevices = navigator.mediaDevices._enumerateDevices
        navigator.mediaDevices.getSupportedConstraints = navigator.mediaDevices._getSupportedConstraints
        navigator.mediaDevices.getUserMedia = navigator.mediaDevices._getUserMedia
    }

}

function _createStopCanvasStreamFunction(stream, meta) {
    return () => {
        window.clearInterval(meta.interval)
        const tracks = stream.getTracks()
        tracks.forEach(track => {
            track.stop()
        })
        if (stream.stop) {
            stream.stop = undefined
        }
    }
}

function createStartedRandomCanvasDrawerInterval(canvas) {
    const FPS = 2
    const ms = 1000 / FPS
    const getRandom = (max) => {
        return Math.floor(Math.random() * max)
    }
    const handle = () => {
        const ctx = canvas.getContext('2d')

        const x = 0
        const y = 0
        const width = getRandom(canvas.width)
        const height = getRandom(canvas.height)

        const r = getRandom(255)
        const g = getRandom(255)
        const b = getRandom(255)

        ctx.fillStyle = `rgb(${r},${g},${b})`

        ctx.fillRect(x, y, width, height)

    }
    const interval = window.setInterval(handle, ms)

    handle()

    return {
        canvas: canvas,
        interval: interval
    }
}

function getMockStreamFromConstraints(constraints) {

    let res = new MediaStream()

    if (constraints['video'] != undefined) {
        let vstream = getMockCanvasStream(constraints);
        let tr = vstream.getVideoTracks();
        res.addTrack(tr[0])
    }


    if (constraints['audio'] === true) {
        const ac = new AudioContext();
        const dest = ac.createMediaStreamDestination();
        let astream = dest.stream;
        let tr = astream.getAudioTracks();
        res.addTrack(tr[0])
    }

    return Promise.resolve(res)
}


function getMockCanvasStream(constraints) {
    const canvas = document.createElement('canvas')

    let constraints_width = constraints['video']['width'] 
    canvas.width = constraints_width == undefined ? 640 : constraints_width

    let constraints_height = constraints['video']['height'] 
    canvas.height = constraints_height == undefined ? 480 : constraints_height

    const meta = createStartedRandomCanvasDrawerInterval(canvas)

    let constraints_fps = constraints['video']['frameRate'] 
    const stream = canvas.captureStream(constraints_fps == undefined ? 20 : constraints_fps)

    stream.stop = _createStopCanvasStreamFunction(stream, meta)
    return stream
}

let mocked = false;
export function mock() {
    if (mocked) {
        restoreOldHandles();
    } else {
        navigator.mediaDevices.getUserMedia = (constraints) => {
            return getMockStreamFromConstraints(constraints)
        }
    }
    mocked = !mocked;
}