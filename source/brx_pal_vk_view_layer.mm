//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <QuartzCore/CALayer.h>

extern void *internal_view_layer(void *void_view)
{
    NSAutoreleasePool *auto_release_pool_view_layer = [[NSAutoreleasePool alloc] init];

    NSView *const view = (NSView *)void_view;
    assert(nil != view);
    CALayer *layer = [view layer];
    assert(nil != layer);

    [auto_release_pool_view_layer release];
    auto_release_pool_view_layer = nil;

    return layer;
}
