#pragma once

#include <QImage>
#include <vector>

#include "obs.h"

// Renders small snapshots of scenes on the OBS graphics thread so the dock
// can show them as thumbnails. Each owner (a DownstreamKeyer) tells the
// renderer which sources it wants; sources nobody wants are dropped.
namespace ThumbnailRenderer {
void Init();
void Shutdown();

void SetSources(void *owner, const std::vector<obs_source_t *> &sources, uint32_t width);
void RemoveOwner(void *owner);

bool GetImage(obs_source_t *source, QImage &image, uint64_t &serial);
} // namespace ThumbnailRenderer
