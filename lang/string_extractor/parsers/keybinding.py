from ..write_text import write_text


def parse_keybinding(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="Key binding name")
