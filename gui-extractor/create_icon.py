#!/usr/bin/env python3
"""
Create a simple PDF document icon for the application
"""
from PIL import Image, ImageDraw, ImageFont
import os

def create_pdf_icon():
    # Create multiple sizes for Windows icon
    sizes = [16, 32, 48, 64, 128, 256]
    images = []

    for size in sizes:
        # Create a new image with transparent background
        img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)

        # Calculate dimensions
        margin = size // 8
        doc_width = size - (2 * margin)
        doc_height = int(doc_width * 1.4)  # A4 ratio
        if doc_height > size - (2 * margin):
            doc_height = size - (2 * margin)
            doc_width = int(doc_height / 1.4)

        # Center the document
        x_offset = (size - doc_width) // 2
        y_offset = (size - doc_height) // 2

        # Draw document shadow
        shadow_offset = max(1, size // 32)
        draw.rounded_rectangle(
            [x_offset + shadow_offset, y_offset + shadow_offset,
             x_offset + doc_width + shadow_offset, y_offset + doc_height + shadow_offset],
            radius=size//16,
            fill=(0, 0, 0, 50)
        )

        # Draw document background
        draw.rounded_rectangle(
            [x_offset, y_offset, x_offset + doc_width, y_offset + doc_height],
            radius=size//16,
            fill=(255, 255, 255, 255),
            outline=(100, 100, 100, 255),
            width=max(1, size//32)
        )

        # Draw folded corner (top-right)
        corner_size = doc_width // 4
        corner_points = [
            (x_offset + doc_width - corner_size, y_offset),
            (x_offset + doc_width, y_offset + corner_size),
            (x_offset + doc_width - corner_size, y_offset + corner_size)
        ]
        draw.polygon(corner_points, fill=(230, 230, 230, 255), outline=(100, 100, 100, 255))

        # Draw "PDF" text if size is large enough
        if size >= 32:
            try:
                # Try to use a built-in font
                font_size = size // 4
                # Use default font
                from PIL import ImageFont
                try:
                    font = ImageFont.truetype("arial.ttf", font_size)
                except:
                    font = ImageFont.load_default()

                text = "PDF"
                # Get text bounding box
                bbox = draw.textbbox((0, 0), text, font=font)
                text_width = bbox[2] - bbox[0]
                text_height = bbox[3] - bbox[1]

                text_x = x_offset + (doc_width - text_width) // 2
                text_y = y_offset + doc_height // 2

                # Draw PDF text in red
                draw.text((text_x, text_y), text, fill=(220, 20, 60, 255), font=font)
            except:
                # If font fails, draw simple lines to represent text
                line_height = max(1, size // 16)
                line_spacing = line_height * 2
                start_y = y_offset + doc_height // 3

                for i in range(3):
                    line_y = start_y + (i * line_spacing)
                    line_width = doc_width - (doc_width // 3)
                    if i == 1:  # Middle line shorter
                        line_width = doc_width // 2
                    draw.rectangle(
                        [x_offset + margin, line_y,
                         x_offset + margin + line_width, line_y + line_height],
                        fill=(200, 200, 200, 255)
                    )

        images.append(img)

    # Save as ICO file - use the pillow-ico package or save individual PNGs
    try:
        images[0].save('app_icon.ico', format='ICO', sizes=[(s, s) for s in sizes], save_all=True)
        print("Created app_icon.ico with sizes:", sizes)
    except:
        # Fallback: save largest size as ICO
        images[-1].save('app_icon.ico')
        print("Created app_icon.ico (single size)")

    # Also save as PNG for reference
    images[-1].save('app_icon.png', format='PNG')
    print("Created app_icon.png (256x256)")

if __name__ == "__main__":
    try:
        from PIL import Image
    except ImportError:
        print("Installing Pillow...")
        os.system("pip install Pillow")
        from PIL import Image, ImageDraw

    create_pdf_icon()