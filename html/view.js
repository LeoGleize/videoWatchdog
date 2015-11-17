window.onload = function() {

  var lanes = ["Freeze","Freeze and no audio","Black Screen","Black Screen and no audio","Live signal","No video","Output not found"];
  var laneLength = lanes.length;
  var timeBegin = 0;
  var timeEnd = 2000;

  function getLaneByEvt(evt){
    for(var i = 0; i < lanes.length; i++)
      if(lanes[i] === evt)
        return i
    return -1;
  }
  allData = [];

  function getColorByEvt(evt){
    if(evt === "Live signal")
      return "green";
    else if(evt === "Freeze")
      return "navy";
    else if(evt === "Output not found")
      return "purple";
    else if(evt === "No video")
      return "red";
    else if(evt === "Black Screen")
      return "gray";
    else if(evt === "Black Screen and no audio")
      return "gray";
    else if(evt === "Freeze and no audio")
      return "blue";
    return "black";
  }

  function drawEventToScreen(id){
    console.log("id = "+id);
    console.log(allData[id]);
    $("#fwhen").text(allData[id].when);
    $("#ftype").text(allData[id].event);
    $("#ftime").text(allData[id].duration_ms);
    $("#fLink").text(allData[id].videoName);
    $("#fLink").attr("href", allData[id].videoName);
  }


  $.ajax({
        dataType: "json",
        url: 'getreport.php',
        success: function( data ) {
          console.log("gotData:");
          console.log(data);
          var items = [];
          realEndTime = 0;
          allData = data.incidents;
          for(var i = 0; i < data.incidents.length; i++){
            obj = {};  
            obj.lane = getLaneByEvt(data.incidents[i].event);
            obj.id = i;
            var parts = data.incidents[i].when.match(/(\d{2}).(\d{2}).(\d{4}) (\d{2}):(\d{2})/);
            obj.start = Date.UTC(+parts[3], parts[2]-1, +parts[1], +parts[4], +parts[5]);
            if(items.length > 0 && obj.start < items[items.length - 1].end)
              obj.start = items[items.length - 1].end;

            obj.end = Math.ceil(obj.start +  data.incidents[i].duration_ms);

            if(i === 0)
              timeBegin = obj.start;
            
          //  obj.start = obj.start - timeBegin;
          //  obj.end = obj.end - timeBegin;
            if(i === data.incidents.length - 1){
              timeEnd = obj.start + data.incidents[i].duration_ms;
              realEndTime = Date.UTC(+parts[3], parts[2]-1, +parts[1], +parts[4], +parts[5]) + data.incidents[i].duration_ms;
            }

            items.push(obj);
          }
          //since data from server have only second resolution and
          //we stacked values that occuped the same time slot we will now 
          //scale it back to fit in real passed time
          var scale = (realEndTime - timeBegin) / (timeEnd - timeBegin);
          for(var i = 0; i < data.incidents.length; i++){
            items[i].start = scale * (items[i].start-timeBegin) + timeBegin;
            items[i].end = scale * (items[i].end-timeBegin) + timeBegin;
          }
          timeEnd = realEndTime;

          console.log(items);
          var m = [20, 15, 15, 180], //top right bottom left
            w = $(document).width() - m[1] - m[3],
            h = 500 - m[0] - m[2],
            miniHeight = laneLength * 12 + 50,
            mainHeight = h - miniHeight - 50;

          //scales
          var x = d3.scale.linear()
              .domain([timeBegin, timeEnd])
              .range([0, w]);
          var x1 = d3.scale.linear()
              .range([0, w]);
          var y1 = d3.scale.linear()
              .domain([0, laneLength])
              .range([0, mainHeight]);
          var y2 = d3.scale.linear()
              .domain([0, laneLength])
              .range([0, miniHeight]);

          var chart = d3.select("body")
                .append("svg")
                .attr("width", w + m[1] + m[3])
                .attr("height", h + m[0] + m[2])
                .attr("class", "chart");
          
          chart.append("defs").append("clipPath")
            .attr("id", "clip")
            .append("rect")
            .attr("width", w)
            .attr("height", mainHeight);

          var main = chart.append("g")
                .attr("transform", "translate(" + m[3] + "," + m[0] + ")")
                .attr("width", w)
                .attr("height", mainHeight)
                .attr("class", "main");

          var mini = chart.append("g")
                .attr("transform", "translate(" + m[3] + "," + (mainHeight + m[0]) + ")")
                .attr("width", w)
                .attr("height", miniHeight)
                .attr("class", "mini");

          var utcScale = d3.time.scale.utc();
          utcScale.domain([timeBegin, timeEnd]).range([0, w]);
          var axis = d3.svg.axis().scale(utcScale);
          chart.append("g")
                .attr('class', 'x axis')
                .attr('transform', 'translate(' + m[3] + ',' + (mainHeight + m[0] * 8) + ')')
                .style('opacity',0.60)
                .call(axis);

          var topTimeScale = d3.time.scale.utc();
          topTimeScale.domain([timeBegin, timeEnd]).range([0, w]);
          var axis = d3.svg.axis().scale(utcScale).orient("top");
          chart.append("g")
                .attr('class', 'x axis')
                .attr('transform', 'translate(' + m[3] + ',' + m[0] + ')')
                // .style('opacity',0.00)
                .attr("id", "topScale")
                .call(axis);

          //main lanes and texts
          main.append("g").selectAll(".laneLines")
            .data(items)
            .enter().append("line")
            .attr("x1", m[1])
            .attr("y1", function(d) {return y1(d.lane);})
            .attr("x2", w)
            .attr("y2", function(d) {return y1(d.lane);})
            .attr("stroke", "lightgray");


          main.append("g").selectAll(".laneText")
            .data(lanes)
            .enter().append("text")
            .text(function(d) {return d;})
            .attr("x", -m[1])
            .attr("y", function(d, i) {return y1(i + .5);})
            .attr("dy", ".5ex")
            .attr("text-anchor", "end")
            .attr("class", "laneText");
          
          //mini lanes and texts
          mini.append("g").selectAll(".laneLines")
            .data(items)
            .enter().append("line")
            .attr("x1", m[1])
            .attr("y1", function(d) {return y2(d.lane);})
            .attr("x2", w)
            .attr("y2", function(d) {return y2(d.lane);})
            .attr("stroke", "lightgray");

          mini.append("g").selectAll(".laneText")
            .data(lanes)
            .enter().append("text")
            .text(function(d) {return d;})
            .attr("x", -m[1])
            .attr("y", function(d, i) {return y2(i + .5);})
            .attr("dy", ".5ex")
            .attr("text-anchor", "end")
            .attr("class", "laneText");

          var itemRects = main.append("g")
                    .attr("clip-path", "url(#clip)");
          
          //mini item rects
          mini.append("g").selectAll("miniItems")
            .data(items)
            .enter().append("rect")
            .attr("class", function(d) {return "miniItem" + d.lane;})
            .attr("x", function(d) {return x(d.start);})
            .attr("y", function(d) {return y2(d.lane + .5) - 5;})
            .attr("width", function(d) {return x(d.end - d.start);})
            .attr("height", 10);

          //mini labels
          mini.append("g").selectAll(".miniLabels")
            .data(items)
            .enter().append("text")
            .text(function(d) {return d.id;})
            .attr("x", function(d) {return x(d.start);})
            .attr("y", function(d) {return y2(d.lane + .5);})
            .attr("dy", ".5ex");

          //brush
          var brush = d3.svg.brush()
                    .x(x)
                    .on("brush", display);

          mini.append("g")
            .attr("class", "x brush")
            .call(brush)
            .selectAll("rect")
            .attr("y", 1)
            .attr("height", miniHeight - 1);

          display();
          
          function display() {
            var rects, labels,
              minExtent = brush.extent()[0],
              maxExtent = brush.extent()[1],
              visItems = items.filter(function(d) {return d.start < maxExtent && d.end > minExtent;});

            mini.select(".brush")
              .call(brush.extent([minExtent, maxExtent]));

            x1.domain([minExtent, maxExtent]);
            console.log(minExtent);
            console.log(maxExtent);
            

            topTimeScale.domain([minExtent, maxExtent]).range([0, w]);
            var axis = d3.svg.axis().scale(topTimeScale).orient("top");
            // chart.append("g")
            //     .attr('class', 'x axis')
            //     .attr('transform', 'translate(' + m[3] + ',' + m[0] + ')')
            //     // .style('opacity',0.00)
            //     // .attr("id", "topScale")
            //     .call(axis);
          // chart.append("g")
          //       .attr('class', 'x axis')
          //       .attr('transform', 'translate(' + m[3] + ',' + m[0] + ')')
          //       // .style('opacity',0.00)
          //       .attr("id", "topScale")
          //       .call(axis);

            d3.select("#topScale").call(axis);
            

            //update main item rects
            rects = itemRects.selectAll("rect")
                    .data(visItems, function(d) { return d.id; })
              .attr("x", function(d) {return x1(d.start);})
              .attr("width", function(d) {return x1(d.end) - x1(d.start);});
            
            rects.enter().append("rect")
              .attr("class", function(d) {return "miniItem" + d.lane;})
              .attr("x", function(d) {return x1(d.start);})
              .attr("y", function(d) {return y1(d.lane) + 10;})
              .attr("width", function(d) {return x1(d.end) - x1(d.start);})
              .attr("height", function(d) {return .8 * y1(1);})
              .on("click",function(d) {return drawEventToScreen(d.id);});

            rects.exit().remove();

            //update the item labels
            labels = itemRects.selectAll("text")
              .data(visItems, function (d) { return d.id; })
              .attr("x", function(d) {return x1(Math.max(d.start, minExtent) + 2);});

            labels.enter().append("text")
              .text(function(d) {return d.id;})
              .attr("x", function(d) {return x1(Math.max(d.start, minExtent));})
              .attr("y", function(d) {return y1(d.lane + .5);})
              .attr("text-anchor", "start");

            labels.exit().remove();
          }
    }
  });
}