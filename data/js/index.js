$(document).ready(function () {

    if ($('#inline').length) {
        $('#inline').minicolors({
            inline: $('#inline').attr('data-inline') === 'true',
            theme: 'bootstrap'
        });
    }

    if ($('#currentDateTime').length) {
        window.setInterval(showDateTime, 900);
    }
});

function setColor() {
    var color = $('#inline').minicolors('rgbObject');
    console.log(color);
    $.post("/color", color);
}

function setBrightness() {
    var brightness = $('#rangeBrightness').val();
    console.log(brightness);
    $.post("/brightness", { brightness: brightness });
}

function updateHourFormat() {
    var hourformat = $('#hourFormat').val();
    $.post("/hourformat", { hourformat: hourformat });
}