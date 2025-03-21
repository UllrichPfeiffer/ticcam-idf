const char camera_index_html[] = R"=====(<!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <title>TicWave Camera TicCAM-1.1kpix</title>
        <style>
            body {
                font-family: Arial,Helvetica,sans-serif;
                background: #181818;
                color: #EFEFEF;
                font-size: 16px
            }

            h2 {
                font-size: 18px
            }

            section.main {
                display: flex
            }

            #menu,section.main {
                flex-direction: column
            }

            #menu {
                display: none;
                flex-wrap: nowrap;
                min-width: 330px;
                background: #363636;
                padding: 8px;
                border-radius: 4px;
                margin-top: -10px;
                margin-right: 10px;
            }

            #content {
                display: flex;
                flex-wrap: wrap;
                align-items: stretch
            }

            figure {
                position: relative;
                display: inline-block;
                padding: 0px;
                margin: 0;
                -webkit-margin-before: 0;
                margin-block-start: 0;
                -webkit-margin-after: 0;
                margin-block-end: 0;
                -webkit-margin-start: 0;
                margin-inline-start: 0;
                -webkit-margin-end: 0;
                margin-inline-end: 0
            }

            figure img {
                display: block;
                width: 100%;
                height: auto;
                border-radius: 0px;
                margin-top: 8px;
            }

            @media (min-width: 800px) and (orientation:landscape) {
                #content {
                    display:flex;
                    flex-wrap: nowrap;
                    align-items: stretch
                }

                figure img {
                    display: block;
                    max-width: 100%;
                    max-height: calc(100vh - 40px);
                    width: auto;
                    height: auto
                }

                figure {
                    padding: 0 0 0 0px;
                    margin: 0;
                    -webkit-margin-before: 0;
                    margin-block-start: 0;
                    -webkit-margin-after: 0;
                    margin-block-end: 0;
                    -webkit-margin-start: 0;
                    margin-inline-start: 0;
                    -webkit-margin-end: 0;
                    margin-inline-end: 0
                }
            }

            section#buttons {
                display: flex;
                flex-wrap: nowrap;
                justify-content: space-between
            }

            #nav-toggle {
                cursor: pointer;
                display: block
            }

            #nav-toggle-cb {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            #nav-toggle-cb:checked+#menu {
                display: flex
            }

            .input-group {
                display: flex;
                flex-wrap: nowrap;
                line-height: 22px;
                margin: 5px 0
            }

            .input-group>label {
                display: inline-block;
                padding-right: 10px;
                min-width: 47%
            }

            .input-group input,.input-group select {
                flex-grow: 1
            }

            .range-max,.range-min {
                display: inline-block;
                padding: 0 5px
            }

            button, .button {
                display: block;
                margin: 5px;
                padding: 0 12px;
                border: 0;
                line-height: 28px;
                cursor: pointer;
                color: #fff;
                background: #009d8b;
                border-radius: 5px;
                font-size: 16px;
                outline: 0
            }

            .save {
                position: absolute;
                right: 25px;
                bottom: -25px;
                height: 16px;
                line-height: 16px;
                padding: 0 4px;
                text-decoration: none;
                cursor: pointer
            }

            .render {
                position: absolute;
                left: 25px;
                bottom: -25px;
                height: 16px;
                line-height: 16px;
                padding: 0 4px;
                text-decoration: none;
                cursor: pointer
            }

            button:hover {
                background: #009d8b
            }

            button:active {
                background: #009d8b
            }

            button.disabled {
                cursor: default;
                background: #a0a0a0
            }

            input[type=range] {
                -webkit-appearance: none;
                width: 100%;
                height: 22px;
                background: #363636;
                cursor: pointer;
                margin: 0
            }

            input[type=range]:focus {
                outline: 0
            }

            input[type=range]::-webkit-slider-runnable-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }

            input[type=range]::-webkit-slider-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #009d8b;
                cursor: pointer;
                -webkit-appearance: none;
                margin-top: -11.5px
            }

            input[type=range]:focus::-webkit-slider-runnable-track {
                background: #EFEFEF
            }

            input[type=range]::-moz-range-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }

            input[type=range]::-moz-range-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #009d8b;
                cursor: pointer
            }

            input[type=range]::-ms-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: 0 0;
                border-color: transparent;
                color: transparent
            }

            input[type=range]::-ms-fill-lower {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }

            input[type=range]::-ms-fill-upper {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }

            input[type=range]::-ms-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #009d8b;
                cursor: pointer;
                height: 2px
            }

            input[type=range]:focus::-ms-fill-lower {
                background: #EFEFEF
            }

            input[type=range]:focus::-ms-fill-upper {
                background: #363636
            }

            .switch {
                display: block;
                position: relative;
                line-height: 22px;
                font-size: 16px;
                height: 22px
            }

            .switch input {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            .slider {
                width: 50px;
                height: 22px;
                border-radius: 22px;
                cursor: pointer;
                background-color: grey
            }

            .slider,.slider:before {
                display: inline-block;
                transition: .4s
            }

            .slider:before {
                position: relative;
                content: "";
                border-radius: 50%;
                height: 16px;
                width: 16px;
                left: 4px;
                top: 3px;
                background-color: #fff
            }

            input:checked+.slider {
                background-color: #009d8b
            }

            input:checked+.slider:before {
                -webkit-transform: translateX(26px);
                transform: translateX(26px)
            }

            select {
                border: 1px solid #363636;
                font-size: 14px;
                height: 22px;
                outline: 0;
                border-radius: 5px
            }

            .image-container {
                position: relative;
                min-width: 160px;
                image-rendering: auto;
            }

            .close {
                position: absolute;
                right: 5px;
                top: 5px;
                background: #009d8b;
                width: 16px;
                height: 16px;
                border-radius: 100px;
                color: #fff;
                text-align: center;
                line-height: 18px;
                cursor: pointer
            }

            .hidden {
                display: none
            }

            input[type=text] {
                border: 1px solid #363636;
                font-size: 14px;
                height: 20px;
                margin: 1px;
                outline: 0;
                border-radius: 5px
            }

            .inline-button {
                line-height: 20px;
                margin: 2px;
                padding: 1px 4px 2px 4px;
            }

            label.toggle-section-label {
                cursor: pointer;
                display: block
            }

            input.toggle-section-button {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            input.toggle-section-button:checked+section.toggle-section {
                display: none
            }

        </style>
    </head>
    <body>
        <section class="main">
            <div id="logo">
              <img src="data:image/png;base64,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" alt="TicWave" height="50">
              <label for="nav-toggle-cb" id="nav-toggle">&#9776;&nbsp;&nbsp;Toggle TicCAM-1.1kpix Settings</label>
            </div>
            <div id="content">
                <div id="sidebar">
                    <input type="checkbox" id="nav-toggle-cb" checked="checked">
                    <nav id="menu">
                        <div class="input-group" id="pclk_lb-group">
                          <label for="pclk_lb">Integration (LB)</label>
                          <div class="range-min">20</div>
                          <input type="range" id="pclk_lb" min="20" max="255" value="100" class="default-action">
                          <div class="range-max">255</div>
                        </div>
                        <div class="input-group" id="special_effect-group">
                            <label for="special_effect">Color Map</label>
                            <select id="special_effect" class="default-action">
                                <option value="0" selected="selected">Grayscale</option>
                                <option value="1">Jetscale</option>
                                <option value="2">Winterscale</option>
                                <option value="3">Autumnscale</option>
                                <option value="4">Springscale</option>
                            </select>
                        </div>
                        <div class="input-group" id="chopper_cyc-group">
                          <label for="chopper_cyc">Chopper Cyc.</label>
                          <select id="chopper_cyc" class="default-action">
                              <option value="0" selected="selected">1</option>
                              <option value="1">2</option>
                              <option value="2">3</option>
                              <option value="3">4</option>
                              <option value="4">5</option>
                              <option value="5">6</option>
                              <option value="6">8</option>
                              <option value="7">9</option>
                              <option value="8">12</option>
                              <option value="9">18</option>
                              <option value="10">36</option>
                          </select>
                      </div>
                      <div class="input-group" id="chopper_cyc-group">
                        <label for="averages">Averages</label>
                        <select id="averages" class="default-action">
                            <option value="0" selected="selected">1</option>
                            <option value="1">4</option>
                            <option value="2">8</option>
                            <option value="3">16</option>
                        </select>
                    </div>
                        <div class="input-group" id="subst-group">
                            <label for="subst">Chopped or CW</label>
                            <div class="switch">
                                <input id="subst" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="subst"></label>
                            </div>
                        </div>
                      <div class="input-group" id="chopper_sync-group">
                        <label for="chopper_sync">Chopper master</label>
                        <div class="switch">
                            <input id="chopper_sync" type="checkbox" class="default-action" checked="checked">
                            <label class="slider" for="chopper_sync"></label>
                        </div>
                      </div>
                        <div class="input-group" id="hmirror-group">
                            <label for="hmirror">H-Mirror</label>
                            <div class="switch">
                                <input id="hmirror" type="checkbox" class="default-action">
                                <label class="slider" for="hmirror"></label>
                            </div>
                        </div>
                        <div class="input-group" id="vflip-group">
                            <label for="vflip">V-Flip</label>
                            <div class="switch">
                                <input id="vflip" type="checkbox" class="default-action">
                                <label class="slider" for="vflip"></label>
                            </div>
                        </div>
                        <div class="input-group" id="rotate-group">
                            <label for="rotate90">Rotate +90</label>
                            <div class="switch">
                                <input id="rotate90" type="checkbox" class="default-action">
                                <label class="slider" for="rotate90"></label>
                            </div>
                        </div>
                        <div class="input-group" id="persistance-group">
                          <label for="autoscale">Infinite Persistance</label>
                          <div class="switch">
                              <input id="persistance" type="checkbox" class="default-action">
                              <label class="slider" for="persistance"></label>
                          </div>
                        </div>
                        <div class="input-group" id="autoscale-group">
                          <label for="autoscale">Autoscale Video</label>
                          <div class="switch">
                              <input id="autoscale" type="checkbox" class="default-action" checked="checked">
                              <label class="slider" for="autoscale"></label>
                          </div>
                        </div>
                        <div class="input-group" id="peak-group">
                          <label for="peak">Peak</label>
                          <div class="switch">
                              <input id="peak" type="checkbox" class="default-action">
                              <label class="slider" for="peak"></label>
                          </div>
                      </div>
                        <div class="input-group" id="zero-group">
                          <label for="zero">Zero</label>
                          <div class="switch">
                              <input id="zero" type="checkbox" class="default-action">
                              <label class="slider" for="zero"></label>
                          </div>
                      </div>
                        <section id="buttons">
                            <button id="get-still">Get Still</button>
                            <button id="toggle-stream">Start Stream</button>
                        </section>
                </div>
                <figure>
                    <div id="stream-container" class="image-container hidden">
                        <a id="save-still" href="#" class="button save" download="capture.jpg">Save</a>
                        <a id="render" href="#" class="button render">Render On</a>
                        <div class="close" id="close-stream">x</div>
                        <img id="stream" src="" crossorigin>
                    </div>
                </figure>
            </div>
        </section>
        <script>
document.addEventListener('DOMContentLoaded', function (event) {
  var baseHost = document.location.origin
  var streamUrl = baseHost + ':81'

  function fetchUrl(url, cb){
    fetch(url)
      .then(function (response) {
        if (response.status !== 200) {
          cb(response.status, response.statusText);
        } else {
          response.text().then(function(data){
            cb(200, data);
          }).catch(function(err) {
            cb(-1, err);
          });
        }
      })
      .catch(function(err) {
        cb(-1, err);
      });
  }

  function setWindow(start_x, start_y, end_x, end_y, offset_x, offset_y, total_x, total_y, output_x, output_y, scaling, binning, cb){
    fetchUrl(`${baseHost}/resolution?sx=${start_x}&sy=${start_y}&ex=${end_x}&ey=${end_y}&offx=${offset_x}&offy=${offset_y}&tx=${total_x}&ty=${total_y}&ox=${output_x}&oy=${output_y}&scale=${scaling}&binning=${binning}`, cb);
  }

  // Attach on change action for register elements
  document
    .querySelectorAll('.reg-action')
    .forEach(el => {
        if (el.type === 'text') {
            el.onkeyup = function(e){
                if(e.keyCode == 13){
                    setRegValue(el);
                }
            }
        } else {
            el.onchange = () => setRegValue(el)
        }
    })


  const hide = el => {
    el.classList.add('hidden')
  }
  const show = el => {
    el.classList.remove('hidden')
  }

  const disable = el => {
    el.classList.add('disabled')
    el.disabled = true
  }

  const enable = el => {
    el.classList.remove('disabled')
    el.disabled = false
  }

  const updateValue = (el, value, updateRemote) => {
    updateRemote = updateRemote == null ? true : updateRemote
    let initialValue
    if (el.type === 'checkbox') {
      initialValue = el.checked
      value = !!value
      el.checked = value
    } else {
      initialValue = el.value
      el.value = value
    }

    if (updateRemote && initialValue !== value) {
      updateConfig(el);
    } else if(!updateRemote){
      if(el.id === "aec"){
        value ? hide(exposure) : show(exposure)
      } else if(el.id === "awb_gain"){
        value ? show(wb) : hide(wb)
      } else if(el.id == "led_intensity"){
        value > -1 ? show(ledGroup) : hide(ledGroup)
      }
    }
  }

  function updateConfig (el) {
    let value
    switch (el.type) {
      case 'checkbox':
        value = el.checked ? 1 : 0
        break
      case 'range':
      case 'select-one':
        value = el.value
        break
      case 'button':
      case 'submit':
        value = '1'
        break
      default:
        return
    }
    
    const query = `${baseHost}/control?var=${el.id}&val=${value}`

    fetch(query)
      .then(response => {
        console.log(`request to ${query} finished, status: ${response.status}`)
      })
  }

  document
    .querySelectorAll('.close')
    .forEach(el => {
      el.onclick = () => {
        hide(el.parentNode)
      }
    })

  // read initial values
  fetch(`${baseHost}/status`)
    .then(function (response) {
      return response.json()
    })
    .then(function (state) {
      document
        .querySelectorAll('.default-action')
        .forEach(el => {
          updateValue(el, state[el.id], false)
        })
      document
        .querySelectorAll('.reg-action')
        .forEach(el => {
            let reg = el.attributes.reg?parseInt(el.attributes.reg.nodeValue):0;
            if(reg == 0){
              return;
            }
            updateRegValue(el, state['0x'+reg.toString(16)], false)
        })
    })

  const view = document.getElementById('stream')
  const viewContainer = document.getElementById('stream-container')
  const stillButton = document.getElementById('get-still')
  const streamButton = document.getElementById('toggle-stream')
  const closeButton = document.getElementById('close-stream')
  const saveButton = document.getElementById('save-still')
  const renderButton = document.getElementById('render')

  view.style.width = "330px";
  view.style.height = "330px";

  const stopStream = () => {
    window.stop();
    streamButton.innerHTML = 'Start Stream'
  }

  const startStream = () => {
    view.src = `${streamUrl}/stream`
    show(viewContainer)
    streamButton.innerHTML = 'Stop Stream'
  }

  // Attach actions to buttons
  stillButton.onclick = () => {
    stopStream()
    view.src = `${baseHost}/capture?_cb=${Date.now()}`
    show(viewContainer)
  }

  closeButton.onclick = () => {
    stopStream()
    hide(viewContainer)
  }

  streamButton.onclick = () => {
    const streamEnabled = streamButton.innerHTML === 'Stop Stream'
    if (streamEnabled) {
      stopStream()
    } else {
      startStream()
    }
  }

  renderButton.onclick = () => {
    const renderEnabled = renderButton.innerHTML === 'Render On'
    if (renderEnabled) {
      view.style.imageRendering = "auto";
      renderButton.innerHTML = 'Render Off'
    } else {
      view.style.imageRendering = "pixelated";
      renderButton.innerHTML = 'Render On'
    }
  }

  saveButton.onclick = () => {
    var canvas = document.createElement("canvas");
    canvas.width = view.width/10;
    canvas.height = view.height/10;
    document.body.appendChild(canvas);
    var context = canvas.getContext('2d');
    context.drawImage(view,0,0);
    try {
      var dataURL = canvas.toDataURL('image/jpeg');
      saveButton.href = dataURL;
      var d = new Date();
      saveButton.download = d.getFullYear() + ("0"+(d.getMonth()+1)).slice(-2) + ("0" + d.getDate()).slice(-2) + ("0" + d.getHours()).slice(-2) + ("0" + d.getMinutes()).slice(-2) + ("0" + d.getSeconds()).slice(-2) + ".jpg";
    } catch (e) {
      console.error(e);
    }
    canvas.parentNode.removeChild(canvas);
  }


  // Attach default on change action
  document
    .querySelectorAll('.default-action')
    .forEach(el => {
      el.onchange = () => updateConfig(el)
    })

  // framesize
  const framesize = document.getElementById('framesize')

  framesize.onchange = () => {
    updateConfig(framesize)
  }

})

        </script>
    </body>
</html>)=====";

size_t camera_index_html_len = sizeof(camera_index_html)-1;
