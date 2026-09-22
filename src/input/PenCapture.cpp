#include "PenCapture.h"

bool PenCapture::initialize(HWND window) {
    return window != nullptr;
}

bool PenCapture::processPointer(UINT, WPARAM wParam) {
    const UINT32 pointerId = GET_POINTERID_WPARAM(wParam);

    POINTER_INPUT_TYPE type{};
    if (!GetPointerType(pointerId, &type) || type != PT_PEN) {
        return false;
    }

    POINTER_PEN_INFO pen{};
    if (!GetPointerPenInfo(pointerId, &pen)) {
        return false;
    }

    current.pointerId = pointerId;
    current.pointerFlags = pen.pointerInfo.pointerFlags;
    current.penFlags = pen.penFlags;
    current.inRange = (pen.pointerInfo.pointerFlags & POINTER_FLAG_INRANGE) != 0;
    current.inContact = (pen.pointerInfo.pointerFlags & POINTER_FLAG_INCONTACT) != 0;
    current.barrel = (pen.penFlags & PEN_FLAG_BARREL) != 0;
    current.pressure = (pen.penMask & PEN_MASK_PRESSURE) != 0 ? pen.pressure : 0;
    current.tiltX = (pen.penMask & PEN_MASK_TILT_X) != 0 ? pen.tiltX : 0;
    current.tiltY = (pen.penMask & PEN_MASK_TILT_Y) != 0 ? pen.tiltY : 0;
    current.rotation = (pen.penMask & PEN_MASK_ROTATION) != 0 ? pen.rotation : 0;
    current.position = pen.pointerInfo.ptPixelLocation;
    return true;
}

void PenCapture::clear() {
    current = {};
}
