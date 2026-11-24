.pragma library

// Noise control icons, after the AirPods mode icons on Apple devices (as in the
// Android app's noise_cancellation, transparency and adaptive images). Rebuilt
// from circles, a ring and four-point stars measured from those images, on a
// 44 px grid with edges on whole pixels so they stay sharp on 1x screens.
// "off" is the person alone. COLOR in an icon's SVG is replaced by its colour.

var size = 44

var icons = {
    "off": {
        bounds: [10, 11, 34, 38], // x0, y0, x1, y1
        svg: '<ellipse cx="22" cy="17.5" rx="6" ry="6.5"/><path d="M10,36 A12,10 0 0 1 34,36 L34,36.5 Q34,38 32.5,38 L11.5,38 Q10,38 10,36.5 Z"/>'
    },
    "noise-cancellation": {
        bounds: [10, 6, 34, 38], // x0, y0, x1, y1
        svg: '<path fill="none" stroke="COLOR" stroke-width="2" stroke-linecap="round" d="M14.71,25.05 A10.5,10.5 0 1 1 29.29,25.05"/><ellipse cx="22" cy="17.5" rx="6" ry="6.5"/><path d="M10,36 A12,10 0 0 1 34,36 L34,36.5 Q34,38 32.5,38 L11.5,38 Q10,38 10,36.5 Z"/>'
    },
    "transparency": {
        bounds: [9, 5, 35, 38], // x0, y0, x1, y1
        svg: '<circle cx="14" cy="25" r="2"/><circle cx="11" cy="19" r="2"/><circle cx="12" cy="13" r="2"/><circle cx="16" cy="9" r="2"/><circle cx="22" cy="7" r="2"/><circle cx="28" cy="9" r="2"/><circle cx="32" cy="13" r="2"/><circle cx="33" cy="19" r="2"/><circle cx="30" cy="25" r="2"/><ellipse cx="22" cy="17.5" rx="6" ry="6.5"/><path d="M10,36 A12,10 0 0 1 34,36 L34,36.5 Q34,38 32.5,38 L11.5,38 Q10,38 10,36.5 Z"/>'
    },
    "adaptive": {
        bounds: [8, 4, 40, 40], // x0, y0, x1, y1
        svg: '<ellipse cx="20" cy="19.5" rx="6" ry="6.5"/><path d="M8,38 A12,10 0 0 1 32,38 L32,38.5 Q32,40 30.5,40 L9.5,40 Q8,40 8,38.5 Z"/><path d="M33.5,7.0 Q34.631,12.369 40.0,13.5 Q34.631,14.631 33.5,20.0 Q32.369,14.631 27.0,13.5 Q32.369,12.369 33.5,7.0 Z"/><path d="M27.5,4.0 Q28.109,6.891 31.0,7.5 Q28.109,8.109 27.5,11.0 Q26.891,8.109 24.0,7.5 Q26.891,6.891 27.5,4.0 Z"/>'
    }
}

// data: URL of the icon drawn in colour (a "#rrggbb" string)
function url(name, colour) {
    const svg = '<svg xmlns="http://www.w3.org/2000/svg" width="' + size + '" height="' + size
        + '" viewBox="0 0 ' + size + ' ' + size + '"><g fill="' + colour + '">'
        + icons[name].svg.replace(/COLOR/g, colour) + '</g></svg>'
    return "data:image/svg+xml;utf8," + encodeURIComponent(svg)
}

// Whole-pixel offset that centres the icon's drawing in its 44 px frame
function offset(name) {
    const b = icons[name].bounds
    return { x: Math.round(size / 2 - (b[0] + b[2]) / 2), y: Math.round(size / 2 - (b[1] + b[3]) / 2) }
}
