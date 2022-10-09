def configureBrokenPlot(ax, ax2):
    # hide the spines between ax and ax2
    ax.spines['bottom'].set_visible(False)
    ax2.spines['top'].set_visible(False)
    ax.xaxis.set_ticks_position('none') 
    ax.tick_params(labeltop=False)  # don't put tick labels at the top
    ax2.xaxis.tick_bottom()

    d = .01  # how big to make the diagonal lines in axes coordinates
    # arguments to pass to plot, just so we don't keep repeating them
    kwargs = dict(transform=ax.transAxes, color='k', clip_on=False)
    ax.plot((-d, +d), (-2.2*d, +2.2*d), **kwargs)        # top-left diagonal
    ax.plot((1 - d, 1 + d), (-2.2*d, +2.2*d), **kwargs)  # top-right diagonal

    kwargs.update(transform=ax2.transAxes)  # switch to the bottom axes
    ax2.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
    ax2.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal

def applyHatches(Benchmarks, hatches_list, axis):
    # Apply hatches to cache bars
    idx = 0
    cnt = 0
    for patch in axis.patches:
        patch.set_hatch(hatches_list[idx])
        cnt += 1
        if cnt >= len(Benchmarks):
            cnt = 0
            idx += 1