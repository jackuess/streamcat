import sys
import urllib.parse
import xml.etree.ElementTree as ET

import requests

NS = {
    "mpd": "urn:mpeg:dash:schema:mpd:2011"
}


def get_adaptation_sets(root):
    periods = root.findall("mpd:Period", NS)
    adaptation_sets = periods[0].findall("mpd:AdaptationSet", NS)
    return adaptation_sets


def parse_template(template):
    ret = ""
    tag = ""
    open = False
    tags = []
    for c in template:
        if c == "$":
            if open:
                c = "}"
                open = False
                if "%" in tag:
                    name, template = tag.split("%", 1)
                else:
                    name = tag
                    template = "s"
                tags.append(name)
                tag = ""
                ret += "%" + template
            else:
                c = "{"
                open = True
        elif open:
            tag += c
        else:
            ret += c
    return ret, tags


def get_segments(adaptation_set):
    segments = []
    representations = adaptation_set.findall("mpd:Representation", NS)
    representation = representations[0]
    segment_template = adaptation_set.find("mpd:SegmentTemplate", NS)
    start_number = int(segment_template.attrib.get("startNumber", "0"))
    segment_timeline = segment_template.find("mpd:SegmentTimeline", NS)
    initialization_template = segment_template.attrib["initialization"]
    media_template = segment_template.attrib["media"]

    times = []
    start = 0
    duration = 0
    for item in segment_timeline.findall("mpd:S", NS):
        start += int(item.attrib.get("t", duration))
        duration = int(item.attrib["d"])
        count = int(item.attrib.get("r", 1))
        for time in range(start, start + (count * duration), duration):
            times.append(time)
            start = time

    initialization_template, initialization_template_tags = parse_template(initialization_template)
    media_template, media_template_tags = parse_template(media_template)
    args = {
        "RepresentationID": representation.attrib["id"]
    }

    segments.append(initialization_template % tuple(
        args[tag] for tag in initialization_template_tags
    ))
    for number, time in enumerate(times, start=start_number):
        args["Time"] = time
        args["Number"] = number
        segments.append(media_template % tuple(
            args[tag] for tag in media_template_tags
        ))

    return segments


if __name__ == "__main__":
    url = sys.argv[1]
    resp = requests.get(url)
    print(resp.text, file=sys.stderr)
    for prev_resp in resp.history:
        url = prev_resp.headers.get("Location", url)
    root = ET.fromstring(resp.text)

    adaptation_sets = get_adaptation_sets(root)
    segments = get_segments(adaptation_sets[0])
    for segment in segments:
        print(urllib.parse.urljoin(url, segment))
