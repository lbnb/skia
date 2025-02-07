/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/ops/DashLinePathRenderer.h"

#include "src/gpu/GrAuditTrail.h"
#include "src/gpu/GrGpu.h"
#include "src/gpu/geometry/GrStyledShape.h"
#include "src/gpu/ops/GrDashOp.h"
#include "src/gpu/ops/GrMeshDrawOp.h"
#include "src/gpu/v1/SurfaceDrawContext_v1.h"

namespace skgpu::v1 {

GrPathRenderer::CanDrawPath DashLinePathRenderer::onCanDrawPath(const CanDrawPathArgs& args) const {
    SkPoint pts[2];
    bool inverted;
    if (args.fShape->style().isDashed() && args.fShape->asLine(pts, &inverted)) {
        // We should never have an inverse dashed case.
        SkASSERT(!inverted);
        if (!GrDashOp::CanDrawDashLine(pts, args.fShape->style(), *args.fViewMatrix)) {
            return CanDrawPath::kNo;
        }
        return CanDrawPath::kYes;
    }
    return CanDrawPath::kNo;
}

bool DashLinePathRenderer::onDrawPath(const DrawPathArgs& args) {
    GR_AUDIT_TRAIL_AUTO_FRAME(args.fContext->priv().auditTrail(),
                              "DashLinePathRenderer::onDrawPath");
    GrDashOp::AAMode aaMode;
    switch (args.fAAType) {
        case GrAAType::kNone:
            aaMode = GrDashOp::AAMode::kNone;
            break;
        case GrAAType::kMSAA:
            // In this mode we will use aa between dashes but the outer border uses MSAA. Otherwise,
            // we can wind up with external edges antialiased and internal edges unantialiased.
            aaMode = GrDashOp::AAMode::kCoverageWithMSAA;
            break;
        case GrAAType::kCoverage:
            aaMode = GrDashOp::AAMode::kCoverage;
            break;
    }
    SkPoint pts[2];
    SkAssertResult(args.fShape->asLine(pts, nullptr));
    GrOp::Owner op =
            GrDashOp::MakeDashLineOp(args.fContext, std::move(args.fPaint), *args.fViewMatrix, pts,
                                     aaMode, args.fShape->style(), args.fUserStencilSettings);
    if (!op) {
        return false;
    }
    args.fSurfaceDrawContext->addDrawOp(args.fClip, std::move(op));
    return true;
}

} // namespace skgpu::v1
