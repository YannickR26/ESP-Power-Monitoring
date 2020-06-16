function timestampToTime(ts) {
    
    function n(n){
        return n > 9 ? "" + n: "0" + n;
    }

    var res = {};
    res.h = n(Math.trunc(ts / 3600));
    res.m = n(Math.trunc((ts - res.h * 3600) / 60));
    res.s = n(Math.trunc(ts - (res.h * 3600 + res.m * 60)));
    return res;
}

function timeToSimestamp(t) {
    if (t.length != 5)
        throw t + "must be formed like 08:15";
    return parseInt(t.substring(0, 2)) * 3600 + parseInt(t.substring(3)) * 60;
}

function animateChart(chart) {
    chart.on('draw', function(data) {
    if(data.type === 'line' || data.type === 'area') {
        data.element.animate({
        d: {
            begin: 2000 * data.index,
            dur: 2000,
            from: data.path.clone().scale(1, 0).translate(0, data.chartRect.height()).stringify(),
            to: data.path.clone().stringify(),
            easing: Chartist.Svg.Easing.easeOutQuint
        }
        });
    }
    
    });
}